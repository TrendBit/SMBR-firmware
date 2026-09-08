#include "fluorometer.hpp"

#include "logger.hpp"
#include "memory.hpp"
#include "threads/fluorometer_thread.hpp"
#include "tools/biquad_filter.hpp"
#include <cstddef>
#include <cstdint>
#include <etl/array.h>

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
    Load_config_data();
}

bool Fluorometer::Load_calibration_data(){
    Logger::Debug("Loading OJIP calibration data...");
    bool read_values_status = memory->Read_OJIP_calibration_values(calibration_data.adc_value);
    bool read_timing_status = memory->Read_OJIP_calibration_timing(calibration_data.timing_us);

    if (read_values_status && read_timing_status) {
        Logger::Debug("OJIP calibration ADC and timing data loaded from memory");
        calibration_data.calibrated = true;
    } else {
        if (!read_values_status) Logger::Error("Failed to load OJIP calibration ADC data from memory");
        if (!read_timing_status) Logger::Error("Failed to load OJIP calibration timing data from memory");
        return false;
    }
    return true;
}

bool Fluorometer::Load_config_data(){
    Logger::Debug("Loading OJIP config data...");
    std::optional<bool> calibration = memory->Read_OJIP_calibration_toggle();
    std::optional<bool> filtering = memory->Read_OJIP_filtering_toggle();

    if(calibration && filtering){
        Logger::Debug("OJIP config data loaded from memory");
        use_calibration = calibration.value_or(true);
        use_filtering = filtering.value_or(true);
    }else{
        if(!calibration.has_value()) Logger::Error("Failed to load OJIP calibration toggle from memory");
        if(!calibration.has_value()) Logger::Error("Failed to load OJIP filtering toggle from memory");
        return false;
    }
    
    return true;
}

void Fluorometer::Calibrate(){
    // Delete old calibration data
    calibration_data.adc_value.fill(0);
    calibration_data.timing_us.fill(0);
    calibration_data.calibrated = false;

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
        calibration_data.calibrated = true;
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

bool Fluorometer::Filtering(){
    return use_filtering;
}

bool Fluorometer::Filtering(bool new_state){
    if(!Capture_done()){
        return false;
    }
    
    use_filtering = new_state;
    return memory->Write_OJIP_config(use_calibration, use_filtering);
}

bool Fluorometer::Calibration(){
    return use_calibration;
}

bool Fluorometer::Calibration(bool new_state){
    if(!Capture_done()){
        return false;
    }
    use_calibration = new_state;
    return memory->Write_OJIP_config(use_calibration, use_filtering);
}

bool Fluorometer::Is_calibrated(){
    return calibration_data.calibrated;
}

bool Fluorometer::Erase_calibration(){
    if (!calibration_data.calibrated){
        return false;
    }
    if(!Capture_done()){
        return false;
    }

    // prevent captures from starting, to decrease chance of loading a corrupted calibration
    ojip_capture_finished = false;

    Logger::Warning("Erasing OJIP calibration data");

    // invalidate current calibration
    calibration_data.calibrated = false;

    bool write_values_status = memory->Erase_OJIP_calibration_timing();
    if (!write_values_status){
        Logger::Error("Unable to erase values data");
    }else{
        Logger::Notice("OJIP values calibration data erased");
    }
    
    bool write_timing_status = memory->Erase_OJIP_calibration_values();
    if (!write_timing_status){
        Logger::Error("Unable to erase values data");
    }else{
        Logger::Notice("OJIP timing calibration data erased");
    }
    
    ojip_capture_finished = true;

    return write_timing_status && write_values_status;
}

bool Fluorometer::Capture_OJIP(Fluorometer_config::Gain gain, float emitor_intensity, float capture_length, uint samples, Fluorometer_config::Timing timing) {
    Logger::Warning("Capture OJIP initiated");

    OJIP_data.sample_count = samples;

    if (!OJIP_phase_0_Preparation(gain, emitor_intensity, capture_length, timing)) {
        return false;
    }

    uint32_t sys_clock_hz = clock_get_hz(clk_sys);
    uint32_t ticks_per_us = sys_clock_hz / 1'000'000 / timer_clock_divider;
    uint32_t max_sample_span_us = (1 << 16) / ticks_per_us;

    // Determine number of samples in fast phase
    int fast_phase_samples = 0;
    for (size_t i = 0; i < capture_timing.size(); i++) {
        uint32_t span = capture_timing[i];
        if (span < max_sample_span_us) {
            fast_phase_samples++;
        } else {
            break;
        }
    }

    int timestamp_dma_channel, wrap_dma_channel, adc_dma_channel;

    if (!OJIP_phase_1_Configuration(timestamp_dma_channel, wrap_dma_channel, adc_dma_channel, fast_phase_samples)) {
        return false;
    }

    uint64_t start_time = OJIP_phase_2_Fast_phase(timestamp_dma_channel, wrap_dma_channel, adc_dma_channel);

    uint64_t stop_time = OJIP_phase_3_Slow_phase(ticks_per_us, fast_phase_samples);

    if (!OJIP_phase_4_Post_processing(start_time, stop_time)) {
        return false;
    }

    return true;
}

bool Fluorometer::OJIP_phase_0_Preparation(Fluorometer_config::Gain gain, float emitor_intensity, float capture_length, Fluorometer_config::Timing timing) {
    Logger::Notice("Initializing memory");
    OJIP_data.sample_time_us.resize(OJIP_data.sample_count);
    OJIP_data.sample_time_us.fill(0);
    OJIP_data.intensity.resize(OJIP_data.sample_count);
    OJIP_data.intensity.fill(0);
    OJIP_data.emitor_intensity = emitor_intensity;
    OJIP_data.detector_gain = gain;
    capture_timing.resize(OJIP_data.sample_count);
    capture_timing.fill(0);

    Logger::Notice("Computing capture timing");

    Generate_timing(capture_timing, OJIP_data.sample_count, capture_length, timing);

    if (capture_timing[1] <= capture_timing[0]) {
        Logger::Error("Capture timing is incorrect, [0]={}, [1]={}", capture_timing[0], capture_timing[1]);
        return false;
    }

    uint32_t sys_clock_hz = clock_get_hz(clk_sys);
    uint32_t ticks_per_us = sys_clock_hz / 1'000'000 / timer_clock_divider;
    // multiply all timing by ticks_per_us for resolution
    for (size_t i = 0; i < capture_timing.size(); i++) {
        capture_timing[i] = capture_timing[i] * ticks_per_us;
    }

    return true;
}

bool Fluorometer::OJIP_phase_1_Configuration(int& timestamp_dma_channel, int& wrap_dma_channel, int& adc_dma_channel, int fast_phase_samples) {
    Logger::Notice("Configuring ADC");
    adc_init();
    adc_gpio_init(26 + 1);
    adc_select_input(1);
    adc_set_clkdiv(0);                          // 0.5 Msps
    adc_fifo_setup(true, true, 1, false, false); // Enable FIFO, 1 sample threshold
    if (adc_fifo_get_level() > 0) {
        adc_fifo_drain();
    }

    // Overclock the ADC to 2,06 Msps
    uint32_t adc_clk_freq_hz = clock_get_hz(clk_sys);
    clock_configure(clk_adc, 0, CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, adc_clk_freq_hz, adc_clk_freq_hz);

    adc_run(true);

    ojip_capture_finished = false;

    Logger::Notice("Setting detector gain");
    if (OJIP_data.detector_gain == Fluorometer_config::Gain::Auto) {
        Logger::Warning("Auto gain not supported, using x1");
        Gain(Fluorometer_config::Gain::x1);
    } else if (OJIP_data.detector_gain == Fluorometer_config::Gain::Undefined) {
        Logger::Error("Undefined gain requested, using x1");
        Gain(Fluorometer_config::Gain::x1);
    } else {
        Gain(OJIP_data.detector_gain);
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
    pwm_config_set_clkdiv_int(&pwm_cfg, timer_clock_divider);
    pwm_init(sampler_trigger_slice, &pwm_cfg, false);
    pwm_set_wrap(sampler_trigger_slice, capture_timing[0]);

    Logger::Notice("Configuring DMA channels");

    timestamp_dma_channel = dma_claim_unused_channel(true);
    wrap_dma_channel      = dma_claim_unused_channel(true);
    adc_dma_channel       = dma_claim_unused_channel(true);

    if (timestamp_dma_channel == -1 || wrap_dma_channel == -1 || adc_dma_channel == -1) {
        Logger::Error("DMA channels not available");
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
    channel_config_set_transfer_data_size(&wrap_dma_config, DMA_SIZE_32);      // 16-bit transfers
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
        fast_phase_samples,                                    // Number of transfers
        true                                        // Start immediately but wait wait for trigger
    );

    // First capture is done immediately at time 0
    dma_channel_configure(
        wrap_dma_channel,
        &wrap_dma_config,
        &pwm_hw->slice[sampler_trigger_slice].top,  // Destination buffer slice threshold
        capture_timing.data(),                      // Source: Timer counter (lower 32 bits)
        fast_phase_samples,                                    // Number of transfers
        true                                        // Start immediately but wait wait for trigger
    );

    dma_channel_configure(
        adc_dma_channel,
        &adc_dma_config,
        OJIP_data.intensity.data(),                 // Destination buffer
        &adc_hw->fifo,                              // Source: ADC fifo with length 1
        fast_phase_samples,                                    // Number of transfers
        true                                        // Start immediately but wait wait for trigger
    );

    return true;
}

uint64_t Fluorometer::OJIP_phase_2_Fast_phase(int timestamp_dma_channel, int wrap_dma_channel, int adc_dma_channel) {
    // Enable emitor
    Emitor_intensity(OJIP_data.emitor_intensity);

    Logger::Notice("Reseting watchdog before capture");
    watchdog_update();

    // Capture time at start of capture
    uint64_t start_time = to_us_since_boot(get_absolute_time());

    // Start trigger timer
    pwm_set_enabled(sampler_trigger_slice, true);

    // Wait for DMA stops -> fast phase capture is done
    while (dma_channel_is_busy(timestamp_dma_channel)) {
        // Actively wait for DMA to complete
    }

    // Deactivate DMA channels
    dma_channel_abort(timestamp_dma_channel);
    dma_channel_abort(wrap_dma_channel);
    dma_channel_abort(adc_dma_channel);

    dma_channel_unclaim(timestamp_dma_channel);
    dma_channel_unclaim(wrap_dma_channel);
    dma_channel_unclaim(adc_dma_channel);

    // Stop PWM trigger
    pwm_set_enabled(sampler_trigger_slice, false);

    // Fast phase finished
    Logger::Notice("Fast phase complete, captured samples");

    return start_time;
}

uint64_t Fluorometer::OJIP_phase_3_Slow_phase(uint32_t ticks_per_us, int fast_phase_samples) {

    // Structure to pass to the lambda
    struct Slow_phase_data {
        uint32_t current_sample_index;
        uint32_t ticks_per_us;
        uint64_t stop_time;
    };

    Slow_phase_data* data = new Slow_phase_data{static_cast<uint32_t>(fast_phase_samples), ticks_per_us, 0};
    uint64_t next_sample_time_us = capture_timing[data->current_sample_index] / data->ticks_per_us;

    Logger::Notice("Next sample at {} us", next_sample_time_us);

    auto Capture_single_sample = [](alarm_id_t id, void *user_data) -> int64_t {
        UNUSED(id);

        Slow_phase_data *data = reinterpret_cast<Slow_phase_data *>(user_data);
        uint32_t *current_sample_index = &data->current_sample_index;
        uint32_t ticks_per_us = data->ticks_per_us;

        // Capture data
        OJIP_data.intensity[*current_sample_index] = adc_fifo_get();
        OJIP_data.sample_time_us[*current_sample_index] = time_us_64();

        (*current_sample_index)++;

        if (*current_sample_index >= OJIP_data.sample_count) {
            data->stop_time = time_us_64();
            return 0;       // No more samples, don't reschedule
        } else {
            return capture_timing[*current_sample_index] / ticks_per_us; // Reschedule next capture
        }
    };

    // Start direct read sampling if there are remaining samples for slow phase
    if (data->current_sample_index < OJIP_data.sample_count) {
        Logger::Notice("Starting slow phase direct read sampling");
        add_alarm_in_us(next_sample_time_us, Capture_single_sample, data, true);
    }

    // Wait for all samples to be captured
    while (data->current_sample_index < OJIP_data.sample_count) {
        rtos::Yield();
    }

    Logger::Debug("Slow phase sampling complete");

    uint64_t stop_time = data->stop_time;  // Retrieve stop_time
    delete data;

    return stop_time;
}

bool Fluorometer::OJIP_phase_4_Post_processing(uint64_t start_time, uint64_t stop_time) {

    Logger::Notice("Stopped DMA channels");

    uint64_t duration  = stop_time - start_time;

    Logger::Notice("Capture finished");

    Logger::Notice("Start time: {:d} us", start_time);
    Logger::Notice("Stop time: {:d} us", stop_time);
    Logger::Notice("Duration: {:d} us", duration);

    Emitor_intensity(0.0f);

    Logger::Notice("Stopping ADC");
    adc_run(false);
    adc_init();

    if (OJIP_data.sample_time_us[0] > OJIP_data.sample_time_us.back()) {
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
        if (use_filtering){
            Logger::Notice("Filtering OJIP data...");
            Filter_OJIP_data(&OJIP_data);
        } else {
            Logger::Notice("Skipping OJIP data filtering...");
        }
    } else {
        Logger::Warning("No valid samples captured, skipping filtering");
    }

    ojip_capture_finished = true;

    return true;
}


/*
 * @brief Helper lambda to compare two numbers, made for qsort.
 */
int compare_int(const void *a, const void *b) {  
    uint16_t va = *static_cast<const uint16_t*>(a);
    uint16_t vb = *static_cast<const uint16_t*>(b);
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

bool Fluorometer::Filter_OJIP_data(OJIP* data) {
    if (!data || data->intensity.empty() || data->sample_time_us.empty()) {
        Logger::Error("Cannot filter OJIP data: null pointer or empty data");
        return false;
    }

    if (data->intensity.size() != data->sample_time_us.size()) {
        Logger::Error("Cannot filter OJIP data: timestamp and intensity size mismatch");
        return false;
    }
    
    Apply_biquad_filter(data, 22000, 0.50);
    Apply_median_filter(data, 3);
    Apply_box_blur_filter(data, 2);
    
    Logger::Notice("OJIP data filtered");
    return true;
}

bool Fluorometer::Apply_biquad_filter(OJIP* data, size_t target_frequency, double q){
    Logger::Notice("Applying biquad filter to OJIP data, target_frequency: {}, q: {}",target_frequency, q);
    
    const BiquadFilter::Type type = BiquadFilter::Type::Lowpass;
    const double fc = target_frequency / 1e6;
    const double max_sample_time = (0.5 / target_frequency) * 1e6;
    const double peak = 3;

    BiquadFilter filter1 = BiquadFilter(type, fc, q, peak);
    BiquadFilter filter2 = BiquadFilter(type, fc, q, peak);
    BiquadFilter filter3 = BiquadFilter(type, fc, q, peak);
    BiquadFilter filter4 = BiquadFilter(type, fc, q, peak);

    size_t last_sample_index = 0;

    // first two values are troublesome because they often contain high intensity, thus they are skipped
    for (size_t i = 2; i < data->intensity.size()-1; i++){
        size_t dt_us = data->sample_time_us[i] - data->sample_time_us[i-1];

        // if sample time is above nyquist frequency, stop filtering
        if (max_sample_time <= dt_us){
            break;
        }
        
        // skip duplicate samples
        if (dt_us == 0){
            data->intensity[i] = data->intensity[i-1];
        }else if (dt_us > 1){
            Logger::Trace("Resampling to {} samples", dt_us);
            // Resample the interval onto a uniform 1 us grid using linear
            // interpolation so the filter always runs at the designed 1 MHz rate
            const int32_t v0 = data->intensity[i-1];
            const int32_t v1 = data->intensity[i];
            for (size_t step = 1; step < dt_us; ++step){
                const uint16_t interp = static_cast<uint16_t>(v0 + (v1 - v0) * static_cast<int32_t>(step) / static_cast<int32_t>(dt_us));
                filter1.process(filter2.process(filter3.process(filter4.process(interp))));
            }
            data->intensity[i] = 
                filter1.process(
                filter2.process(
                filter3.process(
                filter4.process(
                    data->intensity[i]
                ))));
        }else{
            data->intensity[i] = 
                filter1.process(
                filter2.process(
                filter3.process(
                filter4.process(
                    data->intensity[i]
                ))));
        }

        last_sample_index = i;
    }
    
    Logger::Notice("Biquad filter ended at sample {}/{}",last_sample_index,data->intensity.size());
    
    return true;
}

bool Fluorometer::Apply_median_filter(OJIP* data, uint8_t window_span){
    const size_t window_size = (window_span*2) + 1;
    
    Logger::Notice("Applying median filter to OJIP data, window_size: {}",window_size);
    
    // rolling buffer for the original, and still relevant values
    std::vector<uint16_t> window{};
    window.resize(window_size);

    // buffer for sorting the values from window
    std::vector<uint16_t> cache{};
    cache.resize(window_size);
    
    // copy first n values to the window buffer
    std::memcpy(window.data(), &data->intensity[0], window_size * sizeof(uint16_t));
    
    // start with a offset to fit the filter, end with the same offset
    for (size_t i = window_span; (i + window_span) < data->intensity.size(); i++){
        const size_t curr_iteration = i - window_span;
        
        cache = window;
        qsort(cache.data(), window_size, sizeof(uint16_t), compare_int);
        window[curr_iteration%window_size] = data->intensity[i+1];
        data->intensity[i] = cache[window_span+1];
    }
    
    return true;
}

bool Fluorometer::Apply_box_blur_filter(OJIP* data, uint8_t window_span){
    const size_t window_size = (window_span*2) + 1;
    
    Logger::Notice("Applying box blur filter to OJIP data, window_size: {}",window_size);
    
    // rolling buffer for the original, and still relevant values
    std::vector<uint16_t> window{};
    window.resize(window_size);
    
    std::memcpy(window.data(), &data->intensity[0], window_size * sizeof(uint16_t));
    
    // start with a offset to fit the filter, end with the same offset
    for (size_t i = window_span; (i + window_span) < data->intensity.size(); i++){
        const size_t curr_iteration = i - window_span;
        size_t sum = 0;
        for (size_t j = 0; j < window_size; j++){
            sum += window[j];
        }
        
        window[curr_iteration%window_size] = data->intensity[i+1];
        data->intensity[i] = sum/window_size;
    }
    
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
    size_t samples_sent = 0;
    size_t samples_calibrated = 0;

    
    if (not calibration_data.calibrated) {
        Logger::Warning("Exporting {} samples, calibration data invalid or missing",
                        data->sample_time_us.size());
    } else {
        Logger::Notice("Exporting {} samples, calibration based on closest timestamp ({} calibration points) {}",
                        data->sample_time_us.size(), calibration_size, (use_calibration)?"will be applied":"will not be applied");
    }

    Logger::Notice("Reseting watchdog before export");
    watchdog_update();

    float gain_value = 1;
    if (calibration_data.calibrated && use_calibration) {
        // this code sets the gain value (compensation), by searching for the 
        // lowest intensity after 100us (after LED startup), and compares it
        // to the intensity at that time from calibration data. Then, sets the
        // gain value to normalize them.
        // 
        // I expect that the lowest intensity is not tempered by the algae.
        size_t first_stable_index = 0;
        Logger::Notice("searching for sample later than 100us");
        for (size_t i = 0; i < data->sample_time_us.size(); ++i) {
            uint32_t current_time_us = data->sample_time_us[i];
            if (current_time_us > 100){ //about the point where LED intensity stabilises
                first_stable_index = i;
                break;
            }
        }
        Logger::Notice("sample found at index: {}",first_stable_index);

        uint16_t min_intensity = data->intensity[first_stable_index];
        uint16_t min_intensity_time = data->sample_time_us[first_stable_index];
        Logger::Notice("searching for minimum");
        for (size_t i = first_stable_index; i < data->sample_time_us.size()*0.9; ++i) {
            uint16_t current_intensity = data->intensity[i];
            if (current_intensity < min_intensity){
                min_intensity = current_intensity;
                min_intensity_time = i;
            }
        }
        
        Logger::Notice("minimum found at time: {}, value: {}",min_intensity_time, min_intensity);
        
        size_t calibration_index = find_closest_calibration_index(calibration_data.timing_us,min_intensity_time);
        uint16_t calibration_value = calibration_data.adc_value[calibration_index];

        
        Logger::Notice("calibration index: {}, value: {}", calibration_index, calibration_value);
        
        gain_value = static_cast<float>(calibration_value) / static_cast<float>(min_intensity);

        Logger::Notice("Gain compensation: {:.2f}", gain_value);
    }


    

    // Process each captured sample
    for (size_t i = 0; i < data->sample_time_us.size(); ++i) {
        uint32_t current_time_us = data->sample_time_us[i];
        uint16_t current_intensity = data->intensity[i]; // Use raw intensity before filtering if filter applied earlier

        // Apply calibration if available
        if (calibration_data.calibrated && use_calibration) {
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

bool Fluorometer::Generate_timing(etl::vector<uint32_t, FLUOROMETER_MAX_SAMPLES> &capture_timing_us, uint samples, float capture_length, Fluorometer_config::Timing timing_type){
    auto generator = timing_generators.at(timing_type);

    if (generator) {
        return generator(capture_timing_us, samples, capture_length);
    } else {
        Logger::Error("Timing generator not found");
        return false;
    }
}

bool Fluorometer::Timing_generator_logarithmic(etl::vector<uint32_t, FLUOROMETER_MAX_SAMPLES> &capture_timing_us, uint samples, float capture_length){
    if (samples < 2){
        Logger::Error("Cannot generate logarithmic timing, not enough samples");
        return false;
    }
    
    // Calculate the maximum exponent for logarithmic spacing
    const double max_exponent = log10(capture_length * 1e6);

    const double minimal_time_us = 1;


    capture_timing_us[0] = 0;
    double last_time = 0;

    for (uint i = 1; i < samples; i++){
        double exponent = (i * max_exponent) / (samples - 1);
        double current_time = pow(10, exponent);

        // apply minimal gap time and correct current time acordingly
        if ((current_time - last_time) < minimal_time_us) {
            double adjusted_time = last_time + minimal_time_us;
            current_time = pow(10, log10(adjusted_time));
            capture_timing_us[i] = 1;
        }else{
            capture_timing_us[i] = static_cast<uint32_t>(current_time-last_time);
        }
        
        last_time = current_time;
    }


    return true;
}

bool Fluorometer::Timing_generator_linear(etl::vector<uint32_t, FLUOROMETER_MAX_SAMPLES> &capture_timing_us, uint samples, float capture_length){
    uint32_t step = (capture_length / (samples-1)) * 1e6;

    capture_timing_us[0] = 0;
    
    for (unsigned int i = 1; i < samples; ++i) {
        capture_timing_us[i] = step;
    }

    return true;
}
