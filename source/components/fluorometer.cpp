#include "fluorometer.hpp"

Fluorometer::Fluorometer(PWM_channel * led_pwm, uint detector_gain_pin, GPIO * ntc_channel_selector, Thermistor * ntc_thermistors, I2C_bus * const i2c):
    Component(Codes::Component::Fluorometer),
    Message_receiver(Codes::Component::Fluorometer),
    ntc_channel_selector(ntc_channel_selector),
    ntc_thermistors(ntc_thermistors),
    led_pwm(led_pwm),
    detector_gain(new GPIO(detector_gain_pin)),
    detector_adc(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_1, 3.30f)),
    detector_temperature_sensor(new TMP102(*i2c, 0x48))
{
    detector_gain->Set_pulls(false, false);
}

void Fluorometer::Gain(Fluorometer_config::Gain gain){
    switch (gain) {
        case Fluorometer_config::Gain::x1:
            detector_gain->Set_direction(GPIO::Direction::In);
            break;
        case Fluorometer_config::Gain::x10:
            detector_gain->Set_direction(GPIO::Direction::Out);
            detector_gain->Set(false);
            break;
        case Fluorometer_config::Gain::x50:
            detector_gain->Set_direction(GPIO::Direction::Out);
            detector_gain->Set(true);
            break;
        default:
            Logger::Print("Unknown gain requested, settings to x1", Logger::Level::Error);
            Gain(Fluorometer_config::Gain::x1);
            break;
    }
}

uint16_t Fluorometer::Detector_raw_value(){
    return detector_adc->Read_raw();
}

float Fluorometer::Detector_value(uint16_t raw_value){
    return static_cast<float>(raw_value) / ((1<<12)-1);
}

float Fluorometer::Detector_value(){
    return static_cast<float>(detector_adc->Read_raw()) / ((1<<12)-1);
}

void Fluorometer::Emitor_intensity(float intensity){
    led_pwm->Duty_cycle(std::clamp(intensity,0.0f, 1.0f));
}

float Fluorometer::Emitor_intensity(){
    return led_pwm->Duty_cycle();
}

float Fluorometer::Detector_temperature(){
    return detector_temperature_sensor->Temperature();
}

float Fluorometer::Emitor_temperature(){
    ntc_channel_selector->Set(false);
    return ntc_thermistors->Temperature();
}

Fluorometer_config::Gain Fluorometer::Gain(){
    if (detector_gain->Get_direction() == GPIO::Direction::In) {
        return Fluorometer_config::Gain::x1;
    } else if (detector_gain->Read()) {
        return Fluorometer_config::Gain::x50;
    } else {
        return Fluorometer_config::Gain::x10;
    }
}

bool Fluorometer::Capture_OJIP(Fluorometer_config::Gain gain, float emitor_intensity, float capture_length, uint samples, Fluorometer_config::Timing timing){
    Logger::Print("Capture OJIP initiated", Logger::Level::Notice);

    Logger::Print("Initializing memory", Logger::Level::Notice);
    OJIP_data.sample_time_us.resize(samples);
    OJIP_data.intensity.resize(samples);
    capture_timing.resize(samples);
    capture_timing.fill(0);

    Logger::Print("Computing capture timing", Logger::Level::Notice);
    switch (timing) {
        case Fluorometer_config::Timing::Linear:
            Timing_generator_linear(capture_timing, samples, capture_length);
            break;
        case Fluorometer_config::Timing::Logarithmic:
            Timing_generator_logarithmic(capture_timing, samples, capture_length);
            break;
        default:
            Logger::Print("Unknown timing requested, using linear", Logger::Level::Error);
            return 0;
    }

    if (capture_timing[1] <= capture_timing[0]) {
        Logger::Print(emio::format("Capture timing is incorrect, [0]={}, [1]={}",capture_timing[0], capture_timing[1]), Logger::Level::Error);
    }

    Logger::Print("Configuring ADC", Logger::Level::Notice);
    adc_set_clkdiv(0);                          // 0.5 Msps
    adc_fifo_setup(true, true, 1, false, false); // Enable FIFO, 1 sample threshold
    adc_run(true);

    ojip_capture_finished = false;

    Logger::Print("Setting detector gain", Logger::Level::Notice);
    if (gain == Fluorometer_config::Gain::Auto) {
        Logger::Print("Auto gain not supported, using x1", Logger::Level::Warning);
        Gain(Fluorometer_config::Gain::x1);
    } else if (gain == Fluorometer_config::Gain::Undefined) {
        Logger::Print("Undefined gain requested, using x1", Logger::Level::Error);
        Gain(Fluorometer_config::Gain::x1);
    } else {
        Gain(gain);
    }

    uint32_t sys_clock_hz = clock_get_hz(clk_sys);
    Logger::Print(emio::format("System clock: {} Hz", sys_clock_hz), Logger::Level::Notice);

    Logger::Print(emio::format("Timing [0]={}, [1]={}", capture_timing[0], capture_timing[1]), Logger::Level::Notice);

    size_t timing_samples = std::count_if(capture_timing.begin(), capture_timing.end(),
                                         [](uint16_t value) { return value > 0; });

    Logger::Print(emio::format("Valid timing: {:d}", timing_samples), Logger::Level::Notice);

    Logger::Print("Configuring sample trigger slice", Logger::Level::Notice);

    pwm_config pwm_cfg = pwm_get_default_config();
    pwm_set_counter(sampler_trigger_slice, 1);
    pwm_config_set_clkdiv(&pwm_cfg, 125.0);
    pwm_init(sampler_trigger_slice, &pwm_cfg, false);
    pwm_set_wrap(sampler_trigger_slice, capture_timing[0]);

    Logger::Print("Configuring DMA channels", Logger::Level::Notice);

    int timestamp_dma_channel = dma_claim_unused_channel(true);
    int wrap_dma_channel      = dma_claim_unused_channel(true);
    int adc_dma_channel       = dma_claim_unused_channel(true);

    if (timestamp_dma_channel == -1 or wrap_dma_channel == -1 or adc_dma_channel == -1) {
        Logger::Print("DMA channels not available", Logger::Level::Error);
        return false;
    }

    dma_channel_config timestamp_dma_config = dma_channel_get_default_config(timestamp_dma_channel);
    dma_channel_config wrap_dma_config      = dma_channel_get_default_config(wrap_dma_channel);
    dma_channel_config adc_dma_config       = dma_channel_get_default_config(adc_dma_channel);

    uint pwm_channel_dreq = pwm_get_dreq(sampler_trigger_slice);

    // Timestamp from system clock
    channel_config_set_transfer_data_size(&timestamp_dma_config, DMA_SIZE_32); // 32-bit transfers
    channel_config_set_read_increment(&timestamp_dma_config, false);           // Fixed source register
    channel_config_set_write_increment(&timestamp_dma_config, true);           // Increment destination in memory
    channel_config_set_dreq(&timestamp_dma_config, pwm_channel_dreq);          // Trigger DMA by PWM wrap of trigger timer

    // Trigger ADC sample + DMA transfer
    channel_config_set_transfer_data_size(&wrap_dma_config, DMA_SIZE_16);      // 16-bit transfers
    channel_config_set_read_increment(&wrap_dma_config, true);                 // Increment source in memory
    channel_config_set_write_increment(&wrap_dma_config, false);               // Fixed dest register
    channel_config_set_dreq(&wrap_dma_config, pwm_channel_dreq);               // Trigger DMA by PWM wrap of trigger timer

    // ADC samples from detector
    channel_config_set_transfer_data_size(&adc_dma_config, DMA_SIZE_16);       // 16-bit transfers
    channel_config_set_read_increment(&adc_dma_config, false);                 // Fixed source FIFO
    channel_config_set_write_increment(&adc_dma_config, true);                 // Increment destination
    channel_config_set_dreq(&adc_dma_config, pwm_channel_dreq);                // Trigger DMA by PWM wrap of trigger timer

    dma_channel_configure(
        timestamp_dma_channel,
        &timestamp_dma_config,
        OJIP_data.sample_time_us.data(),            // Destination buffer
        &timer_hw->timerawl,                        // Source: Timer counter (lower 32 bits), increments every 1 us
        samples,                                    // Number of transfers
        true                                        // Start immediately but wait wait for trigger
    );

    // First capture is done immediately at time 0
    dma_channel_configure(
        wrap_dma_channel,
        &wrap_dma_config,
        &pwm_hw->slice[sampler_trigger_slice].top,  // Destination buffer slice threshold
        capture_timing.data()+2,                     // Source: Timer counter (lower 32 bits)
        samples,                                    // Number of transfers
        true                                        // Start immediately but wait wait for trigger
    );

    dma_channel_configure(
        adc_dma_channel,
        &adc_dma_config,
        OJIP_data.intensity.data(),                 // Destination buffer
        &adc_hw->fifo,                              // Source: ADC fifo with length 1
        samples,                                    // Number of transfers
        true                                        // Start immediately but wait wait for trigger
    );

    Logger::Print("Starting capture", Logger::Level::Notice);
    rtos::Delay(10);

    Emitor_intensity(emitor_intensity);

    uint64_t start_time = to_us_since_boot(get_absolute_time());

    // Start trigger timer
    pwm_set_enabled(sampler_trigger_slice, true);

    // Wait DMA stops -> curve capture is done
    while (dma_channel_is_busy(timestamp_dma_channel)) {
        rtos::Delay(5);
    }

    dma_channel_abort(timestamp_dma_channel);
    dma_channel_abort(wrap_dma_channel);
    dma_channel_abort(adc_dma_channel);

    dma_channel_unclaim(timestamp_dma_channel);
    dma_channel_unclaim(wrap_dma_channel);
    dma_channel_unclaim(adc_dma_channel);

    Logger::Print("Stopped DMA channels", Logger::Level::Notice);

    uint64_t stop_time = to_us_since_boot(get_absolute_time());
    uint64_t duration  = stop_time - start_time;

    Logger::Print("Capture finished", Logger::Level::Notice);

    Logger::Print(emio::format("Start time: {:d} us", start_time), Logger::Level::Notice);
    Logger::Print(emio::format("Stop time: {:d} us", stop_time), Logger::Level::Notice);
    Logger::Print(emio::format("Duration: {:d} us", duration), Logger::Level::Notice);

    Emitor_intensity(0.0f);

    Logger::Print("Stopping ADC", Logger::Level::Notice);
    adc_run(false);
    adc_init();

    float temperature = Emitor_temperature();
    Logger::Print(emio::format("Emitor temperature: {:05.2f}°C", temperature), Logger::Level::Notice);

    if(OJIP_data.sample_time_us[0] > OJIP_data.sample_time_us.back()){
        Logger::Print("Timer crosses 32-bit boundary, needs adjusting", Logger::Level::Warning);
    }

    bool timer_overflow = Process_timestamps(start_time, OJIP_data.sample_time_us);
    if (timer_overflow) {
        Logger::Print("Timer overflow detected, adjusting", Logger::Level::Warning);
    }

    size_t samples_captured = std::count_if(OJIP_data.intensity.begin(), OJIP_data.intensity.end(),
                                         [](uint16_t value) { return value > 0; });

    Logger::Print(emio::format("Valid samples: {:d}", samples_captured), Logger::Level::Notice);

    // Print_curve_data(&OJIP_data);

    ojip_capture_finished = true;

    return true;
}



void Fluorometer::Print_curve_data(OJIP * data){
    for (size_t i = 0; i < data->sample_time_us.size(); i++) {
        Logger::Print(emio::format("{:8d} {:04d}", data->sample_time_us[i], data->intensity[i]));
    }
}

bool Fluorometer::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Fluorometer::Export_data(OJIP * data){
    App_messages::Fluorometer::Data_sample sample;
    sample.measurement_id = data->measurement_id;
    sample.gain = data->detector_gain;
    sample.emitor_intensity = data->emitor_intensity;

    if (data->sample_time_us.size() == data->intensity.size()) {
        for (size_t i = 0; i < data->sample_time_us.size(); ++i) {
            const auto& time = data->sample_time_us[i];
            const auto& intensity = data->intensity[i];
            sample.time_us = time;
            sample.sample_value = intensity;
            uint queue = Send_CAN_message(sample);
            if (queue < 32) {
                Logger::Print(emio::format("CAN queue filling up, size:{}", queue), Logger::Level::Notice);
                if (queue < 8) {
                    rtos::Delay(1);
                }
            }
        }
    } else {
        Logger::Print("OJIP sample intensity and timestamp vectors have different sizes", Logger::Level::Error);
    }
    return true;
}

bool Fluorometer::Receive(Application_message message){
    switch (message.Message_type()) {
        case Codes::Message_type::Fluorometer_sample_request: {
            Logger::Print("Fluorometer sample request", Logger::Level::Notice);
            App_messages::Fluorometer::Sample_request sample_request;

            if (not sample_request.Interpret_data(message.data)) {
                Logger::Print("Fluorometer sample request interpretation failed", Logger::Level::Error);
                return false;
            }

            uint8_t measurement_id = sample_request.measurement_id;
            Gain(sample_request.detector_gain);
            Emitor_intensity(sample_request.emitor_intensity);
            rtos::Delay(50);

            uint16_t sample_value = Detector_raw_value();

            Logger::Print(emio::format("Sample value: {:5.3f}, raw: {:4d}", Detector_value(sample_value), sample_value), Logger::Level::Notice);

            App_messages::Fluorometer::Sample_response sample_response;
            sample_response.measurement_id = measurement_id;
            sample_response.sample_value = sample_value;
            sample_response.gain = Gain();
            sample_response.emitor_intensity = Emitor_intensity();

            Send_CAN_message(sample_response);
            Emitor_intensity(0.0);
            return true;
        }

        case Codes::Message_type::Fluorometer_OJIP_capture_request: {
            Logger::Print("Fluorometer OJIP Capture", Logger::Level::Notice);
            App_messages::Fluorometer::OJIP_capture_request ojip_request;
            ojip_request.Interpret_data(message.data);

            if (not ojip_request.Interpret_data(message.data)) {
                Logger::Print("Fluorometer OJIP Capture interpretation failed", Logger::Level::Error);
                return false;
            }

            if (not ojip_capture_finished) {
                Logger::Print("Fluorometer OJIP Capture in progress", Logger::Level::Warning);
                return false;
            }

            Logger::Print(emio::format("Starting capture with gain: {:2.0f}, intensity: {:04.2f}, length: {:3.1f}s, samples: {:d}",
                                        Fluorometer_config::gain_values.at(ojip_request.detector_gain),
                                        ojip_request.emitor_intensity,
                                        (ojip_request.length_ms/1000.0f),
                                        ojip_request.samples
            ));

            OJIP_data.measurement_id = ojip_request.measurement_id;
            OJIP_data.emitor_intensity = ojip_request.emitor_intensity;

            Capture_OJIP(ojip_request.detector_gain, ojip_request.emitor_intensity, (ojip_request.length_ms/1000.0f), ojip_request.samples, ojip_request.sample_timing);

            OJIP_data.detector_gain = Gain();

            return true;
        }

        case Codes::Message_type::Fluorometer_OJIP_completed_request: {
            Logger::Print("Fluorometer OJIP finished request", Logger::Level::Notice);
            App_messages::Fluorometer::OJIP_completed_response response(ojip_capture_finished);
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Fluorometer_OJIP_retrieve_request: {
            Logger::Print("Fluorometer OJIP retrieve request", Logger::Level::Notice);
            return Export_data(&OJIP_data);
        }

        case Codes::Message_type::Fluorometer_detector_info_request: {
            Logger::Print("Fluorometer detector info request", Logger::Level::Notice);
            App_messages::Fluorometer::Detector_info_response response(700, 1);
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Fluorometer_detector_temperature_request: {
            Logger::Print("Fluorometer detector temperature request", Logger::Level::Notice);
            float temp = Detector_temperature();
            Logger::Print(emio::format("Detector temperature: {:05.2f}°C", temp));
            App_messages::Fluorometer::Detector_temperature_response response(temp);
            Send_CAN_message(response);
            return true;
        }

                case Codes::Message_type::Fluorometer_emitor_info_request: {
            Logger::Print("Fluorometer emitor info request", Logger::Level::Notice);
            App_messages::Fluorometer::Emitor_info_response response(535, 10000);
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Fluorometer_emitor_temperature_request: {
            Logger::Print("Fluorometer emitor temperature request", Logger::Level::Notice);
            float temp = Emitor_temperature();
            Logger::Print(emio::format("LED temperature: {:05.2f}°C", Emitor_temperature()));
            App_messages::Fluorometer::Emitor_temperature_response response(temp);
            Send_CAN_message(response);
            return true;
        }

        default:
            return false;
    }
}

bool Fluorometer::Process_timestamps(uint64_t start, etl::vector<uint32_t, FLUOROMETER_MAX_SAMPLES> &sample_time_us){
    bool timer_overflow = false;
    for (size_t i = 0; i < sample_time_us.size(); i++) {
        if (sample_time_us[i] < start) {
            timer_overflow = true;
        }
        sample_time_us[i] = sample_time_us[i] - start;
    }
    return timer_overflow;
}

bool Fluorometer::Timing_generator_logarithmic(etl::vector<uint16_t, FLUOROMETER_MAX_SAMPLES> &capture_timing_us, uint samples, float capture_length){
    // Calculate the maximum exponent for logarithmic spacing
    double max_exponent = log10(capture_length * 1e6);

    double minimal_time_us = 1;
    std::vector<float> timings(samples,0.0f);

    // Generate sampling times in microseconds
    timings[0] = 0.0;               // Time of the start
    timings[1] = minimal_time_us;   // Time of first sample

    for (unsigned int i = 2; i < samples; ++i) {
        double exponent = (i * max_exponent) / (samples - 1);
        double current_time = pow(10, exponent);

        // Apply the minimum time gap between samples
        if ((current_time - timings[i - 1]) < minimal_time_us) {
            double adjusted_time = timings[i - 1] + minimal_time_us;
            timings[i] = pow(10, log10(adjusted_time));
        } else {
            timings[i] = current_time;
        }
    }

    // Convert timings to clock cycles of source
    capture_timing_us[0] = 0;
    for (unsigned int i = 1; i < samples; ++i) {
        capture_timing_us[i] = static_cast<uint32_t>(timings[i]-timings[i-1]);
    }

    /*
    for (unsigned int i = 0; i < samples; ++i) {
        Logger::Print(emio::format("Timing: {:10.1f} {:10d}", timings[i], capture_timing_us[i]));
    }*/

    return true;
}

bool Fluorometer::Timing_generator_linear(etl::vector<uint16_t, FLUOROMETER_MAX_SAMPLES> &capture_timing_us, uint samples, float capture_length){
    double step = capture_length / samples;
    for (unsigned int i = 0; i < samples; ++i) {
        capture_timing_us[i] = static_cast<uint16_t>(step * i * 1e6);
    }

    return true;
}
