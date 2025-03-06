#include "fluorometer.hpp"

Fluorometer::Fluorometer(PWM_channel * led_pwm, uint detector_gain_pin, GPIO * ntc_channel_selector, Thermistor * ntc_thermistors):
    Component(Codes::Component::Fluorometer),
    Message_receiver(Codes::Component::Fluorometer),
    ntc_channel_selector(ntc_channel_selector),
    ntc_thermistors(ntc_thermistors),
    led_pwm(led_pwm),
    detector_gain(new GPIO(detector_gain_pin)),
    detector_adc(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_1, 3.30f))
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

bool Fluorometer::Capture_OJIP(Fluorometer_config::Gain gain, float emitor_intensity, float capture_length, uint samples){
    Logger::Print("Capture OJIP initiated", Logger::Level::Notice);

    Logger::Print("Initializing memory", Logger::Level::Notice);
    OJIP_data.sample_time_us.resize(samples);
    OJIP_data.intensity.resize(samples);
    capture_timing.resize(samples);
    capture_timing.fill(0);

    Logger::Print("Computing capture timing", Logger::Level::Notice);

    if (capture_timing[1] <= capture_timing [0]) {
        Logger::Print("Capture timing is incorrect", Logger::Level::Error);
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

    dma_channel_configure(
        wrap_dma_channel,
        &wrap_dma_config,
        &pwm_hw->slice[sampler_trigger_slice].top,  // Destination buffer slice threshold
        capture_timing.data(),                     // Source: Timer counter (lower 32 bits)
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

    uint64_t start_time = to_us_since_boot(get_absolute_time());

    Logger::Print("Starting capture", Logger::Level::Notice);

    Emitor_intensity(emitor_intensity);

    // Start trigger timer
    pwm_set_enabled(sampler_trigger_slice, true);

    // Wait DMA stops -> curve capture is done
    while (dma_channel_is_busy(timestamp_dma_channel)) {
        rtos::Delay(1);
    }

    uint64_t stop_time = to_us_since_boot(get_absolute_time());
    uint64_t duration  = stop_time - start_time;

    Logger::Print("Capture finished", Logger::Level::Notice);

    Logger::Print(emio::format("Start time: {:d} us", start_time), Logger::Level::Notice);
    Logger::Print(emio::format("Stop time: {:d} us", stop_time), Logger::Level::Notice);
    Logger::Print(emio::format("Duration: {:d} us", duration), Logger::Level::Notice);

    Emitor_intensity(emitor_intensity);

    float temperature = Emitor_temperature();

    Logger::Print(emio::format("Emitor temperature: {:05.2f}°C", temperature), Logger::Level::Notice);


    Logger::Print("Stopping DMA channels", Logger::Level::Notice);
    dma_channel_abort(timestamp_dma_channel);
    dma_channel_abort(wrap_dma_channel);
    dma_channel_abort(adc_dma_channel);

    dma_channel_unclaim(timestamp_dma_channel);
    dma_channel_unclaim(wrap_dma_channel);
    dma_channel_unclaim(adc_dma_channel);

    Logger::Print("Stopping ADC", Logger::Level::Notice);
    adc_run(false);

    if(OJIP_data.sample_time_us[0] > OJIP_data.sample_time_us.back()){
        Logger::Print("Timer crosses 32-bit boundary, needs adjusting", Logger::Level::Warning);
    }

    Print_curve_data(& OJIP_data);

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

bool Fluorometer::Receive(Application_message message){
    switch (message.Message_type()) {
        case Codes::Message_type::Fluorometer_sample_request: {
            Logger::Print("Fluorometer sample request", Logger::Level::Notice);
            App_messages::Fluorometer::Sample_request sample_request;
            sample_request.Interpret_data(message.data);
            Logger::Print(emio::format("LED temperature: {:05.2f}°C", Emitor_temperature()));
            Emitor_intensity(1.0);
            Gain(sample_request.gain);
            rtos::Delay(500);
            uint16_t sample_value = Detector_raw_value();
            Logger::Print(emio::format("Sample value: {:05.3f}", Detector_value(sample_value)), Logger::Level::Notice);
            App_messages::Fluorometer::Sample_response sample_response;
            sample_response.sample_value = sample_value;
            sample_response.gain = sample_request.gain;
            Send_CAN_message(sample_response);
            Logger::Print(emio::format("LED temperature: {:05.2f}°C", Emitor_temperature()));
            Emitor_intensity(0.0);

            return true;
        }

        default:
            return false;
    }
}


