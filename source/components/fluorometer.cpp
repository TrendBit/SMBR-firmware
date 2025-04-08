#include "fluorometer.hpp"

Fluorometer::Fluorometer(PWM_channel * led_pwm, uint detector_gain_pin, GPIO * ntc_channel_selector, Thermistor * ntc_thermistors, I2C_bus * const i2c, EEPROM_storage * const memory):
    Component(Codes::Component::Fluorometer),
    Message_receiver(Codes::Component::Fluorometer),
    ntc_channel_selector(ntc_channel_selector),
    ntc_thermistors(ntc_thermistors),
    led_pwm(led_pwm),
    detector_gain(new GPIO(detector_gain_pin)),
    detector_adc(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_1, 3.30f)),
    detector_temperature_sensor(new TMP102(*i2c, 0x48)),
    memory(memory)
{
    detector_gain->Set_pulls(true, true);
    Gain(Fluorometer_config::Gain::x1);
    calibration_data.adc_value.fill(0);
    memory->Read_OJIP_calibration(calibration_data.adc_value);
}

void Fluorometer::Calibrate(){

    // Initialize calibration data
    bool stat = Capture_OJIP(Fluorometer_config::Gain::x50, 1.0, 1.0, 1000, Fluorometer_config::Timing::Logarithmic);

    if (!stat) {
        Logger::Print("Capture OJIP failed", Logger::Level::Error);
        return;
    }

    // Copy captured value to calibration data
    for(size_t i = 0; i < calibration_data.adc_value.size(); i++){
        calibration_data.adc_value[i] = OJIP_data.intensity[i];
    }

    Logger::Print("Current calibration data", Logger::Level::Notice);
    for ( size_t i = 0; i < calibration_data.adc_value.size(); i++){
        Logger::Print(emio::format("{:d} = {:d}", i, calibration_data.adc_value[i]), Logger::Level::Notice);
    }

    memory->Write_OJIP_calibration(calibration_data.adc_value);

    calibration_data.adc_value.fill(0);
    Logger::Print("Calibration data written to memory", Logger::Level::Notice);

    // Read calibration data from memory
    memory->Read_OJIP_calibration(calibration_data.adc_value);

    Logger::Print("Calibration data read from memory", Logger::Level::Notice);
    for ( size_t i = 0; i < calibration_data.adc_value.size(); i++){
        Logger::Print(emio::format("{:d} = {:d}", i, calibration_data.adc_value[i]), Logger::Level::Notice);
    }
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
    led_pwm->Reset_counter();
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
    OJIP_data.sample_time_us.fill(0);
    OJIP_data.intensity.resize(samples);
    OJIP_data.intensity.fill(0);
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
    adc_init();
    adc_gpio_init(26 + 1);
    adc_select_input(1);
    adc_set_clkdiv(0);                          // 0.5 Msps
    adc_fifo_setup(true, true, 1, false, false); // Enable FIFO, 1 sample threshold
    if(adc_fifo_get_level() > 0){
        adc_fifo_drain();
    }
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
    pwm_set_counter(sampler_trigger_slice, 0);
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

    rtos::Delay(1000);

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
        capture_timing.data(),                      // Source: Timer counter (lower 32 bits)
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

    pwm_set_enabled(sampler_trigger_slice, false);

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

    // Filter OJIP data
    if (samples_captured > 0) {
        Filter_OJIP_data(&OJIP_data, 1.0f);
    } else {
        Logger::Print("No valid samples captured, skipping filtering", Logger::Level::Warning);
    }

    ojip_capture_finished = true;

    return true;
}

bool Fluorometer::Filter_OJIP_data(OJIP* data, float tau_ms) {
    if (!data || data->intensity.empty() || data->sample_time_us.empty()) {
        Logger::Print("Cannot filter OJIP data: null pointer or empty data", Logger::Level::Error);
        return false;
    }

    if (data->intensity.size() != data->sample_time_us.size()) {
        Logger::Print("Cannot filter OJIP data: timestamp and intensity size mismatch", Logger::Level::Error);
        return false;
    }

    // Create temporary buffer for filtered values
    std::vector<uint16_t> filtered(data->intensity.size());

    // Initialize filter with first value
    filtered[0] = data->intensity[0];
    float y = static_cast<float>(data->intensity[0]);

    // Apply exponential filter with irregular time intervals
    for (size_t i = 1; i < data->intensity.size(); ++i) {
        // Calculate time difference in milliseconds
        float dt_ms = static_cast<float>(data->sample_time_us[i] - data->sample_time_us[i-1]) / 1000.0f;

        // Safety check for time differences
        if (dt_ms <= 0.0f) dt_ms = 0.001f;  // minimum 1µs

        // Calculate time dilation factor
        float time_dilation_factor = static_cast<float>(data->sample_time_us[i]) / 10000.0f;
        if (time_dilation_factor < 0.1f) time_dilation_factor = 0.1f;

        // Calculate smoothing coefficient
        float alpha = 1.0f - std::exp(-dt_ms / (tau_ms * time_dilation_factor));

        // Apply filter
        y = alpha * static_cast<float>(data->intensity[i]) + (1.0f - alpha) * y;

        // Store filtered value
        filtered[i] = static_cast<uint16_t>(std::round(y));
    }

    // Copy filtered values back to the original buffer
    for (size_t i = 0; i < data->intensity.size(); ++i) {
        data->intensity[i] = filtered[i];
    }

    Logger::Print(emio::format("OJIP data filtered with tau={:.1f}ms", tau_ms), Logger::Level::Notice);
    return true;
}


void Fluorometer::Print_curve_data(OJIP * data){
    for (size_t i = 0; i < data->sample_time_us.size(); i++) {
        Logger::Print_raw(emio::format("{:8d} {:04d}\r\n", data->sample_time_us[i], data->intensity[i]));
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

    const uint base_estimation_length = 5;

    uint sample_base;
    uint calibration_base;

    uint sample_peak = 0;
    uint calibration_peak = 0;

    uint accumulator = 0;

    int offset = 0;

    for(size_t i = 0; i < base_estimation_length; ++i) {
        accumulator += data->intensity[i];
    }
    sample_base = accumulator / base_estimation_length;

    for(size_t i = 0; i < base_estimation_length; ++i) {
        accumulator += calibration_data.adc_value[i];
    }
    calibration_base = accumulator / base_estimation_length;

    int i;
    for(i = 0; i < 1000; i++) {
        if(data->intensity[i] > (sample_base*5)){
            sample_peak = i;
            break;
        }
    }

    for(i = 0; i < 1000; i++) {
        if(calibration_data.adc_value[i] > (calibration_base*2)){
            calibration_peak = i;
            break;
        }
    }

    float gain_value = 50.0f / Fluorometer_config::gain_values.at(data->detector_gain);

    Logger::Print(emio::format("Gain compensation: {:.2f}", gain_value), Logger::Level::Notice);

    offset = sample_peak - calibration_peak;

    Logger::Print(emio::format("Sample base: {:d}, calibration base: {:d}, offset: {:d}", sample_base, calibration_base, offset), Logger::Level::Notice);

    if (data->sample_time_us.size() == data->intensity.size()) {
        // Perform some initial setup calculations
        const size_t calibration_size = calibration_data.adc_value.size();
        const bool has_calibration = calibration_size > 0;
        size_t samples_sent = 0;
        size_t samples_calibrated = 0;

        Logger::Print(emio::format("Exporting {} samples with offset {}",
                    data->sample_time_us.size(), offset), Logger::Level::Notice);

        // Process each sample
        for (size_t i = 0; i < data->sample_time_us.size(); ++i) {
            // Prepare sample message
            sample.time_us = data->sample_time_us[i];
            sample.sample_value = data->intensity[i];
            sample.measurement_id = data->measurement_id;
            sample.gain = data->detector_gain;
            sample.emitor_intensity = data->emitor_intensity;

            // Apply calibration if available
            if (has_calibration && i > 1) { // Skip first samples
                int cal_idx = static_cast<int>(i) - offset;

                // Ensure index is in valid range
                if (cal_idx >= 0 && cal_idx < static_cast<int>(calibration_size)) {
                    // Get calibration value for this index
                    uint16_t correction = calibration_data.adc_value[cal_idx] / gain_value;

                    // Apply correction with underflow protection
                    if (sample.sample_value > correction) {
                        sample.sample_value -= correction;
                        samples_calibrated++;
                    } else {
                        sample.sample_value = 0;
                    }
                } else if (cal_idx >= static_cast<int>(calibration_size)) {
                    // Use last available calibration value
                    uint16_t correction = calibration_data.adc_value[calibration_size - 1] / gain_value;

                    if (sample.sample_value > correction) {
                        sample.sample_value -= correction;
                    } else {
                        sample.sample_value = 0;
                    }
                }
                // For negative indices, we don't apply calibration as those are initial samples
            }

            // Send message with CAN queue management
            uint queue = Send_CAN_message(sample);
            samples_sent++;

            // Manage CAN queue to prevent overflow
            if (queue > 48) {
                // CAN queue getting full, add progressive delays
                rtos::Delay(1);

                if ((i % 20) == 0) {
                    Logger::Print("CAN queue high level", Logger::Level::Warning);
                }

            }
        }

        Logger::Print(emio::format("OJIP export complete: {}/{} samples sent, {} calibrated",
                    samples_sent, data->sample_time_us.size(), samples_calibrated), Logger::Level::Notice);
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

            //Capture_OJIP(ojip_request.detector_gain, ojip_request.emitor_intensity, (ojip_request.length_ms/1000.0f), ojip_request.samples, ojip_request.sample_timing);

            Capture_OJIP(ojip_request.detector_gain, 1.0, 1.0, 1000, Fluorometer_config::Timing::Logarithmic);

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
            if(!ojip_capture_finished) {
                Logger::Print("Fluorometer OJIP capture not finished", Logger::Level::Warning);
                return true;
            }

            if(OJIP_data.intensity.size() == 0) {
                Logger::Print("Fluorometer OJIP data empty", Logger::Level::Warning);
                return true;
            }

            return Export_data(&OJIP_data);
        }

        case Codes::Message_type::Fluorometer_detector_info_request: {
            Logger::Print("Fluorometer detector info request", Logger::Level::Notice);
            App_messages::Fluorometer::Detector_info_response response(700, 1, 500);
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

        case Codes::Message_type::Fluorometer_calibration_request: {
            Logger::Print("Fluorometer calibration request", Logger::Level::Notice);
            Calibrate();
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
    // Print timing data
    for (unsigned int i = 0; i < samples; ++i) {
        Logger::Print_raw(emio::format("Timing: {:10.1f} {:10d} \r\n", timings[i], capture_timing_us[i]));
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
