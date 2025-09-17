#include "fluorometer.hpp"

#include "memory.hpp"
#include "threads/fluorometer_thread.hpp"

Fluorometer::Fluorometer(PWM_channel * led_pwm, uint detector_gain_pin, GPIO * ntc_channel_selector, Thermistor * ntc_thermistors, I2C_bus * const i2c, EEPROM_storage * const memory, fra::MutexStandard * cuvette_mutex, fra::MutexStandard * const adc_mutex):
    Component(Codes::Component::Fluorometer),
    Message_receiver(Codes::Component::Fluorometer),
    ntc_channel_selector(ntc_channel_selector),
    ntc_thermistors(ntc_thermistors),
    led_pwm(led_pwm),
    detector_gain(new GPIO(detector_gain_pin)),
    detector_adc(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_1, 3.30f)),
    detector_temperature_sensor(new TMP102(*i2c, 0x48)),
    memory(memory),
    fluorometer_thread(new Fluorometer_thread(this)),
    cuvette_mutex(cuvette_mutex),
    adc_mutex(adc_mutex)
{
    detector_gain->Set_pulls(true, true);
    Gain(Fluorometer_config::Gain::x10);
    Load_calibration_data();
}

bool Fluorometer::Load_calibration_data(){
    Logger::Debug("Loading OJIP calibration data...");
    bool read_values_status = memory->Read_OJIP_calibration_values(calibration_data.adc_value);
    bool read_timing_status = memory->Read_OJIP_calibration_timing(calibration_data.timing_us);

    if (read_values_status && read_timing_status) {
        Logger::Debug("OJIP calibration ADC and timing data loaded from memory");
    } else {
        if (!read_values_status) Logger::Error("Failed to load OJIP calibration ADC data from memory");
        if (!read_timing_status) Logger::Error("Failed to load OJIP calibration timing data from memory");
        return false;
    }
    return true;
}

void Fluorometer::Calibrate(){

    // Initialize calibration data
    bool stat = Capture_OJIP(
        calibration_data.gain,
        calibration_data.intensity,
        calibration_data.length,
        calibration_data.sample_count,
        calibration_data.timing);

    if (!stat) {
        Logger::Error("Capture OJIP failed");
        return;
    }

    // Copy captured value and timings to calibration data
    for(size_t i = 0; i < calibration_data.adc_value.size(); i++){
        calibration_data.adc_value[i] = OJIP_data.intensity[i];
        calibration_data.timing_us[i] = OJIP_data.sample_time_us[i];
    }

    Logger::Notice("Current calibration data");
    /*
    for ( size_t i = 0; i < calibration_data.adc_value.size(); i++){
        Logger::Notice("{:d} = {:d}", i, calibration_data.adc_value[i]);
    }*/

    Logger::Notice("Writing calibration data to EEPROM...");
    bool write_values_status = memory->Write_OJIP_calibration_values(calibration_data.adc_value);
    bool write_timing_status = memory->Write_OJIP_calibration_timing(calibration_data.timing_us);

    if (write_values_status && write_timing_status) {
        Logger::Notice("Calibration ADC and timing data written to memory successfully");
    } else {
         if (!write_values_status) Logger::Error("Failed to write calibration ADC data to memory");
         if (!write_timing_status) Logger::Error("Failed to write calibration timing data to memory");
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
            Logger::Error("Unknown gain requested, settings to x1");
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

std::optional<float> Fluorometer::Emitor_temperature(){
    bool lock = adc_mutex->Lock(0);
    if (!lock) {
        Logger::Warning("Fluorometer emitor temperature ADC mutex lock failed");
        return std::nullopt;
    }
    ntc_channel_selector->Set(false);
    float temp = ntc_thermistors->Temperature();
    adc_mutex->Unlock();
    return temp;
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
    Logger::Notice("Capture OJIP initiated");

    Logger::Notice("Initializing memory");
    OJIP_data.sample_time_us.resize(samples);
    OJIP_data.sample_time_us.fill(0);
    OJIP_data.intensity.resize(samples);
    OJIP_data.intensity.fill(0);
    capture_timing.resize(samples);
    capture_timing.fill(0);

    Logger::Notice("Computing capture timing");

    Generate_timing(capture_timing, samples, capture_length, timing);

    if (capture_timing[1] <= capture_timing[0]) {
        Logger::Error("Capture timing is incorrect, [0]={}, [1]={}",capture_timing[0], capture_timing[1]);
    }

    Logger::Notice("Reseting watchdog before capture");
    watchdog_update();

    Logger::Notice("Configuring ADC");
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

    Logger::Notice("Setting detector gain");
    if (gain == Fluorometer_config::Gain::Auto) {
        Logger::Warning("Auto gain not supported, using x1");
        Gain(Fluorometer_config::Gain::x1);
    } else if (gain == Fluorometer_config::Gain::Undefined) {
        Logger::Error("Undefined gain requested, using x1");
        Gain(Fluorometer_config::Gain::x1);
    } else {
        Gain(gain);
    }

    uint32_t sys_clock_hz = clock_get_hz(clk_sys);
    Logger::Notice("System clock: {} Hz", sys_clock_hz);

    Logger::Notice("Timing [0]={}, [1]={}", capture_timing[0], capture_timing[1]);

    size_t timing_samples = std::count_if(capture_timing.begin(), capture_timing.end(),
                                         [](uint16_t value) { return value > 0; });

    Logger::Notice("Valid timing: {:d}", timing_samples);

    Logger::Notice("Configuring sample trigger slice");

    pwm_config pwm_cfg = pwm_get_default_config();
    pwm_set_counter(sampler_trigger_slice, 0);
    // Increment timer every 1us
    pwm_config_set_clkdiv(&pwm_cfg, clock_get_hz(clk_sys) / 10000000.0f);
    pwm_init(sampler_trigger_slice, &pwm_cfg, false);
    pwm_set_wrap(sampler_trigger_slice, capture_timing[0]);

    Logger::Notice("Configuring DMA channels");

    int timestamp_dma_channel = dma_claim_unused_channel(true);
    int wrap_dma_channel      = dma_claim_unused_channel(true);
    int adc_dma_channel       = dma_claim_unused_channel(true);

    if (timestamp_dma_channel == -1 or wrap_dma_channel == -1 or adc_dma_channel == -1) {
        Logger::Error("DMA channels not available");
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

    Logger::Notice("Stopped DMA channels");

    uint64_t stop_time = to_us_since_boot(get_absolute_time());
    uint64_t duration  = stop_time - start_time;

    Logger::Notice("Capture finished");

    Logger::Notice("Start time: {:d} us", start_time);
    Logger::Notice("Stop time: {:d} us", stop_time);
    Logger::Notice("Duration: {:d} us", duration);

    Emitor_intensity(0.0f);

    Logger::Notice("Stopping ADC");
    adc_run(false);
    adc_init();

    if(OJIP_data.sample_time_us[0] > OJIP_data.sample_time_us.back()){
        Logger::Warning("Timer crosses 32-bit boundary, needs adjusting");
    }

    bool timer_overflow = Process_timestamps(start_time, OJIP_data.sample_time_us);
    if (timer_overflow) {
        Logger::Warning("Timer overflow detected");
    }

    size_t samples_captured = std::count_if(OJIP_data.intensity.begin(), OJIP_data.intensity.end(),
                                         [](uint16_t value) { return value > 0; });

    Logger::Notice("Valid samples: {:d}", samples_captured);

    // Print_curve_data(&OJIP_data);

    // Filter OJIP data
    if (samples_captured > 0) {
        Filter_OJIP_data(&OJIP_data, 1.0f);
    } else {
        Logger::Warning("No valid samples captured, skipping filtering");
    }

    ojip_capture_finished = true;

    return true;
}

bool Fluorometer::Filter_OJIP_data(OJIP* data, float tau_ms) {
    if (!data || data->intensity.empty() || data->sample_time_us.empty()) {
        Logger::Error("Cannot filter OJIP data: null pointer or empty data");
        return false;
    }

    if (data->intensity.size() != data->sample_time_us.size()) {
        Logger::Error("Cannot filter OJIP data: timestamp and intensity size mismatch");
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

    Logger::Notice("OJIP data filtered with tau={:.1f}ms", tau_ms);
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

size_t find_closest_calibration_index(std::array<uint32_t, FLUOROMETER_CALIBRATION_SAMPLES> calibration_data, uint32_t target_time_us) {
    const auto& cal_times = calibration_data;
    const size_t cal_size = cal_times.size();

    if (cal_size == 0) {
        return 0; // No calibration data
    }

    // Use lower_bound to find the first element not less than target_time_us
    auto it = std::lower_bound(cal_times.begin(), cal_times.end(), target_time_us);

    // Handle edge cases: target_time_us is before the first element
    if (it == cal_times.begin()) {
        return 0;
    }

    // Handle edge cases: target_time_us is after the last element
    if (it == cal_times.end()) {
        return cal_size - 1;
    }

    // We are between two elements: *(it-1) and *it
    // Check which one is closer
    uint32_t time_before = *(it - 1);
    uint32_t time_after = *it;
    size_t index_before = std::distance(cal_times.begin(), it - 1);

    if ((target_time_us - time_before) < (time_after - target_time_us)) {
        // Closer to the element before
        return index_before;
    } else {
        // Closer to the element at or after (or equally close)
        return index_before + 1;
    }
}

bool Fluorometer::Export_data(OJIP * data){
   App_messages::Fluorometer::Data_sample sample;
    sample.measurement_id = data->measurement_id;
    sample.gain = data->detector_gain;
    sample.emitor_intensity = data->emitor_intensity;

    if (data->sample_time_us.size() != data->intensity.size()) {
         Logger::Error("OJIP sample intensity and timestamp vectors have different sizes");
         return false;
    }

    const size_t calibration_size = calibration_data.adc_value.size();
    const bool has_calibration = calibration_size > 0 && calibration_data.timing_us.size() == calibration_size;
    size_t samples_sent = 0;
    size_t samples_calibrated = 0;

    if (!has_calibration) {
        Logger::Warning("Calibration data invalid or missing, exporting raw data.");
    } else {
        Logger::Notice("Exporting {} samples, applying calibration based on closest timestamp ({} calibration points)",
                     data->sample_time_us.size(), calibration_size);
    }

    Logger::Notice("Reseting watchdog before export");
    watchdog_update();

    float gain_value = Fluorometer_config::gain_values.at(calibration_data.gain) / Fluorometer_config::gain_values.at(data->detector_gain);

    Logger::Notice("Gain compensation: {:.2f}", gain_value);


    // Process each captured sample
    for (size_t i = 0; i < data->sample_time_us.size(); ++i) {
        uint32_t current_time_us = data->sample_time_us[i];
        uint16_t current_intensity = data->intensity[i]; // Use raw intensity before filtering if filter applied earlier

        // Apply calibration if available
        if (has_calibration) {
            // Find the index in calibration data with the closest timestamp
            size_t cal_idx = find_closest_calibration_index(calibration_data.timing_us,current_time_us);

            // Get the corresponding calibration ADC value
            uint16_t correction = calibration_data.adc_value[cal_idx] / gain_value;

            // Apply correction with underflow protection
            if (current_intensity > correction) {
                current_intensity -= correction;
                samples_calibrated++;
            } else {
                current_intensity = 0;
            }

            // Optional: Log the mapping occasionally for debugging
            if (i < 5 || i % 200 == 0 || i == data->sample_time_us.size() - 1) {
                 Logger::Trace("Sample {:4d} (t={:8d}us) mapped to Calib {:4d} (t={:8d}us), Corr: {:4d}",
                               i, current_time_us, cal_idx, calibration_data.timing_us[cal_idx], correction);
            }
        }

        // Prepare the CAN message
        sample.time_us = current_time_us;
        sample.sample_value = current_intensity; // Use the (potentially) calibrated value

        // Send message and manage CAN queue
        uint queue = Send_CAN_message(sample);
        samples_sent++;

        // Manage CAN queue to prevent overflow
        if (queue > 48) {
            rtos::Delay(1);
            if ((i % 100) == 0) {
                Logger::Warning("CAN queue high level");
            }
        }
    }

    Logger::Notice("OJIP export complete: {}/{} samples sent, {} calibrated",
                samples_sent, data->sample_time_us.size(), samples_calibrated);

    return true;
}

bool Fluorometer::Receive(Application_message message){
    switch (message.Message_type()) {
        case Codes::Message_type::Fluorometer_sample_request: {
            Logger::Notice("Fluorometer sample request");
            App_messages::Fluorometer::Sample_request sample_request;

            if (not sample_request.Interpret_data(message.data)) {
                Logger::Error("Fluorometer sample request interpretation failed");
                return false;
            }

            uint8_t measurement_id = sample_request.measurement_id;
            Gain(sample_request.detector_gain);
            Emitor_intensity(sample_request.emitor_intensity);
            rtos::Delay(50);

            uint16_t sample_value = Detector_raw_value();

            Logger::Notice("Sample value: {:5.3f}, raw: {:4d}", Detector_value(sample_value), sample_value);

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
            Logger::Notice("Fluorometer OJIP Capture request enqueued");
            return fluorometer_thread->Enqueue_message(message);
        }

        case Codes::Message_type::Fluorometer_OJIP_completed_request: {
            Logger::Notice("Fluorometer OJIP finished request");
            App_messages::Fluorometer::OJIP_completed_response response(ojip_capture_finished);
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Fluorometer_OJIP_retrieve_request: {
            Logger::Notice("Fluorometer OJIP retrieve request enqueued");
            return fluorometer_thread->Enqueue_message(message);
        }

        case Codes::Message_type::Fluorometer_detector_info_request: {
            Logger::Notice("Fluorometer detector info request");
            App_messages::Fluorometer::Detector_info_response response(700, 1, 500);
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Fluorometer_detector_temperature_request: {
            Logger::Notice("Fluorometer detector temperature request");
            float temp = Detector_temperature();
            Logger::Debug("Detector temperature: {:05.2f}°C", temp);
            App_messages::Fluorometer::Detector_temperature_response response(temp);
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Fluorometer_emitor_info_request: {
            Logger::Notice("Fluorometer emitor info request");
            App_messages::Fluorometer::Emitor_info_response response(535, 10000);
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Fluorometer_emitor_temperature_request: {
            Logger::Notice("Fluorometer emitor temperature request");
            auto temp = Emitor_temperature();
            if (!temp) {
                Logger::Error("Fluorometer emitor temperature not available");
                return false;
            }
            Logger::Debug("LED temperature: {:05.2f}°C", temp.value());
            App_messages::Fluorometer::Emitor_temperature_response response(temp.value());
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Fluorometer_calibration_request: {
            Logger::Notice("Fluorometer calibration request enqueued");
            fluorometer_thread->Enqueue_message(message);
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

bool Fluorometer::Generate_timing(etl::vector<uint16_t, FLUOROMETER_MAX_SAMPLES> &capture_timing_us, uint samples, float capture_length, Fluorometer_config::Timing timing_type){
    auto generator = timing_generators.at(timing_type);

    if (generator) {
        return generator(capture_timing_us, samples, capture_length);
    } else {
        Logger::Error("Timing generator not found");
        return false;
    }
}

bool Fluorometer::Timing_generator_logarithmic(etl::vector<uint16_t, FLUOROMETER_MAX_SAMPLES> &capture_timing_us, uint samples, float capture_length){
    // Calculate the maximum exponent for logarithmic spacing
    double max_exponent = log10(capture_length * 1e6);

    double minimal_time_us = 3;
    std::vector<float> timings(samples, 0.0f);

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

    // Convert timings to clock cycles of source between capture
    capture_timing_us[0] = 0;
    for (unsigned int i = 1; i < samples; ++i) {
        capture_timing_us[i] = static_cast<uint32_t>(timings[i]-timings[i-1]);
    }


    // Print timing data
    /*
    for (unsigned int i = 0; i < samples; ++i) {
        Logger::Print_raw(emio::format("Timing: {:10.1f} {:10d} \r\n", timings[i], capture_timing_us[i]));
    }
    */

    return true;
}

bool Fluorometer::Timing_generator_linear(etl::vector<uint16_t, FLUOROMETER_MAX_SAMPLES> &capture_timing_us, uint samples, float capture_length){
    double step = capture_length / (samples-1);

    std::vector<float> timings(samples, 0.0f);

    for (unsigned int i = 0; i < samples; ++i) {
        timings[i] = step * i * 1e6;
    }

    capture_timing_us[0] = 0;
    for (unsigned int i = 1; i < samples; ++i) {
        capture_timing_us[i] = static_cast<uint16_t>(timings[i]-timings[i-1]);
    }

    // Print timing data
    /*
    for (unsigned int i = 0; i < samples; ++i) {
        Logger::Print_raw(emio::format("Timing: {:10.1f} {:10d} \r\n", timings[i], capture_timing_us[i]));
    }
    */

    return true;
}
