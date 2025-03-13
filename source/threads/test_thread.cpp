#include "test_thread.hpp"
#include "can_bus/can_message.hpp"
#include "can_bus/message_router.hpp"
#include "modules/base_module.hpp"
#include "modules/sensor_module.hpp"
#include "modules/control_module.hpp"

#include "components/led/led_pwm.hpp"
#include "components/led_panel.hpp"
#include "components/fan/fan_gpio.hpp"
#include "components/fan/fan_pwm.hpp"
#include "components/fan/fan_rpm.hpp"
#include "components/motors/dc_hbridge.hpp"

#include "hardware/pwm.h"

#include "rtos/delayed_execution.hpp"

#include "hal/i2c/i2c_bus.hpp"
#include "components/adc/TLA2024.hpp"
#include "components/adc/TLA2024_channel.hpp"
#include "components/thermometers/thermistor.hpp"
#include "components/thermometers/thermopile.hpp"
#include "components/led/KTD2026.hpp"
#include "components/photodetectors/VEML6040.hpp"
#include "components/memory/AT24Cxxx.hpp"
#include "components/thermometers/TMP102.hpp"

#include "logger.hpp"
#include "emio/emio.hpp"

Test_thread::Test_thread(CAN_thread *can_thread)
    : Thread("test_thread", 4096, 10),
    can_thread(can_thread){
    Start();
};

Test_thread::Test_thread()
    : Thread("test_thread", 4096, 10){
    Start();
};

void Test_thread::Run(){
    Logger::Print("Test thread init");

    I2C_bus *i2c = new I2C_bus(i2c1, 10, 11, 100000, true);
    // Calibrate_VEML_lux(*i2c);
    // Spectrophotometer_test(*i2c);
    // LED_test(*i2c);

    // Fluoro_buck_test();
};


void Test_thread::Calibrate_VEML_lux(I2C_bus &i2c){

    KTD2026 *led_driver_0 = new KTD2026(i2c, 0x31);
    led_driver_0->Init();

    KTD2026 *led_driver_1 = new KTD2026(i2c, 0x30);
    led_driver_1->Init();

    VEML6040 *rgbw_sensor = new VEML6040(i2c, 0x10);

    rgbw_sensor->Exposure_time(VEML6040::Exposure::_1280_ms);


    led_driver_0->Intensity(KTD2026::Channel::CH_3, 1.0);

    rtos::Delay(2000);

    float green = rgbw_sensor->Measure_relative(VEML6040::Channels::Green);
    float white = rgbw_sensor->Measure_relative(VEML6040::Channels::White);

    float white_compensation = green / white;

    Logger::Print(emio::format("Green/white values: {:05.3f}/{:05.3f}", green, white));
    Logger::Print(emio::format("White compensation: {:05.3f}", white_compensation));

    led_driver_0->Intensity(KTD2026::Channel::CH_3, 0.0f);

    rgbw_sensor->Exposure_time(VEML6040::Exposure::_320_ms);

    led_driver_1->Intensity(KTD2026::Channel::CH_1, 1.0f);
    rtos::Delay(800);

    float red = rgbw_sensor->Measure_relative(VEML6040::Channels::Red);
    white = rgbw_sensor->Measure_relative(VEML6040::Channels::White);

    float red_compensation = white / red;

    Logger::Print(emio::format("White/red values: {:05.3f}/{:05.3f}", white, red));
    Logger::Print(emio::format("Red compensation: {:05.3f}", red_compensation));

    led_driver_1->Intensity(KTD2026::Channel::CH_1, 0.0f);

    rgbw_sensor->Exposure_time(VEML6040::Exposure::_320_ms);
    led_driver_0->Intensity(KTD2026::Channel::CH_2, 1.0f);
    rtos::Delay(800);

    float blue = rgbw_sensor->Measure_relative(VEML6040::Channels::Blue);
    white = rgbw_sensor->Measure_relative(VEML6040::Channels::White);

    float blue_compensation = white / blue;

    Logger::Print(emio::format("White/blue values: {:05.3f}/{:05.3f}", white, blue));
    Logger::Print(emio::format("Blue compensation: {:05.3f}", blue_compensation));

    led_driver_0->Intensity(KTD2026::Channel::CH_2, 0.0f);
}

void Test_thread::EEPROM_Test(I2C_bus &i2c){
    auto eeprom = new AT24Cxxx(i2c, 0x50, 256);

    auto read = eeprom->Read(0, 2);

    if (read.has_value()) {
        Logger::Print(emio::format("EEPROM read: 0x{:02x} 0x{:02x}", read.value()[0], read.value()[1]));
    } else {
        Logger::Print("EEPROM read failed");
    }

    if (eeprom->Write(0, { 0x01, 0x02 })) {
        Logger::Print("EEPROM write success");
    } else {
        Logger::Print("EEPROM write failed");
    }

    read = eeprom->Read(0, 2);

    if (read.has_value()) {
        Logger::Print(emio::format("EEPROM read: 0x{:02x} 0x{:02x}", read.value()[0], read.value()[1]));
    } else {
        Logger::Print("EEPROM read failed");
    }
}

void Test_thread::Thermopile_test(I2C_bus &i2c){
    TLA2024 *adc = new TLA2024(i2c, 0x4b);

    TLA2024_channel *adc_ch0 = new TLA2024_channel(adc, TLA2024::Channels::AIN0_GND);
    TLA2024_channel *adc_ch1 = new TLA2024_channel(adc, TLA2024::Channels::AIN1_GND);
    TLA2024_channel *adc_ch2 = new TLA2024_channel(adc, TLA2024::Channels::AIN2_GND);
    TLA2024_channel *adc_ch3 = new TLA2024_channel(adc, TLA2024::Channels::AIN3_GND);

    Thermistor *thp_ntc = new Thermistor(adc_ch0, 3960, 100000, 25, 100000, 3.3f);

    Thermopile *thermopile_top    = new Thermopile(adc_ch1, adc_ch0, 0.95);
    Thermopile *thermopile_bottom = new Thermopile(adc_ch3, adc_ch2, 0.95);

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1000));

        Logger::Print(emio::format("Thermistor top    voltage: {:05.3f}V", adc_ch0->Read_voltage()));
        Logger::Print(emio::format("Thermopile top    voltage: {:05.3f}V", adc_ch1->Read_voltage()));
        Logger::Print(emio::format("Thermistor bottom voltage: {:05.3f}V", adc_ch2->Read_voltage()));
        Logger::Print(emio::format("Thermopile bottom voltage: {:05.3f}V", adc_ch3->Read_voltage()));
        Logger::Print("-------");
        Logger::Print(emio::format("Thermistor top    temperature: {:05.2f}°C", thermopile_top->Ambient()));
        Logger::Print(emio::format("Thermopile top    temperature: {:05.2f}°C", thermopile_top->Temperature()));
        Logger::Print(emio::format("Thermistor bottom temperature: {:05.2f}°C", thermopile_bottom->Ambient()));
        Logger::Print(emio::format("Thermopile bottom temperature: {:05.2f}°C", thermopile_bottom->Temperature()));
        Logger::Print("-------");
    }
}

void Test_thread::Fluoro_boost_test(){
    GPIO *LED_en  = new GPIO(22, GPIO::Direction::Out);
    GPIO *LED_pwm = new GPIO(25, GPIO::Direction::Out);

    LED_en->Set(true);
    LED_pwm->Set(true);

    rtos::Delay(2000);

    LED_pwm->Set(false);
    LED_en->Set(false);
}

void Test_thread::Fluoro_buck_test(){
    GPIO *NTC_select = new GPIO(18, GPIO::Direction::Out);
    NTC_select->Set(false);

    auto ntc_adc = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_3, 3.30f);

    Thermistor *flr_ntc = new Thermistor(ntc_adc, 3950, 10000, 25, 5100, 3.3f);

    Logger::Print(emio::format("LED temperature: {:05.2f}°C", flr_ntc->Temperature()));

    rtos::Delay(500);

    Logger::Print(emio::format("LED temperature: {:05.2f}°C", flr_ntc->Temperature()));

    rtos::Delay(500);

    Logger::Print(emio::format("LED temperature: {:05.2f}°C", flr_ntc->Temperature()));

    auto led_pwm = new PWM_channel(23, 100000, 0.0, true);
    led_pwm->Duty_cycle(1.0);

    rtos::Delay(2000);

    led_pwm->Duty_cycle(0.0);

    while(true){
        DelayUntil(fra::Ticks::MsToTicks(500));
        Logger::Print(emio::format("LED temperature: {:05.2f}°C", flr_ntc->Temperature()));
    }
}

void Test_thread::RGBW_sensor_test(I2C_bus &i2c){
    VEML6040 *rgb_sensor = new VEML6040(i2c, 0x10);

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1000));

        uint16_t det_red = rgb_sensor->Measure(VEML6040::Channels::Red);
        rtos::Delay(10);
        uint16_t det_green = rgb_sensor->Measure(VEML6040::Channels::Green);
        rtos::Delay(10);
        uint16_t det_blue = rgb_sensor->Measure(VEML6040::Channels::Blue);
        rtos::Delay(10);
        uint16_t det_white = rgb_sensor->Measure(VEML6040::Channels::White);
        rtos::Delay(10);

        Logger::Print(emio::format("RGBW: {:5d} {:5d} {:5d} {:5d}", det_red, det_green, det_blue, det_white));
    }
}

void Test_thread::Temp_sensor_test(I2C_bus &i2c){
    GPIO *NTC_select = new GPIO(18, GPIO::Direction::Out);
    NTC_select->Set(true);

    auto ntc_adc = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_3, 3.30f);
    auto ntc     = new Thermistor(ntc_adc, 3950, 10000, 25, 5100);

    RP_internal_temperature *mcu_internal_temp = new RP_internal_temperature(3.30f);

    TMP102 *temp_1 = new TMP102(i2c, 0x48);
    TMP102 *temp_2 = new TMP102(i2c, 0x4a);
    TMP102 *temp_3 = new TMP102(i2c, 0x49);

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1000));

        Logger::Print(emio::format("Board temperature: {:05.2f}°C", ntc->Temperature()));
        Logger::Print(emio::format("MCU temperature: {:05.2f}°C", mcu_internal_temp->Temperature()));
        Logger::Print(emio::format("M2_inner temperature: {:05.2f}°C", temp_1->Temperature()));
        Logger::Print(emio::format("M2_outer temperature: {:05.2f}°C", temp_2->Temperature()));
        Logger::Print(emio::format("M3_driver temperature: {:05.2f}°C", temp_3->Temperature()));
        Logger::Print("-------");
    }
}

void Test_thread::LED_test(I2C_bus &i2c){
    KTD2026 *led_driver_0 = new KTD2026(i2c, 0x31);
    led_driver_0->Init();

    KTD2026 *led_driver_1 = new KTD2026(i2c, 0x30);
    led_driver_1->Init();

    led_driver_0->Intensity(KTD2026::Channel::CH_1, 1.0f);
    led_driver_0->Intensity(KTD2026::Channel::CH_2, 1.0f);
    led_driver_0->Intensity(KTD2026::Channel::CH_3, 1.0f);

    led_driver_1->Intensity(KTD2026::Channel::CH_1, 1.0f);
    led_driver_1->Intensity(KTD2026::Channel::CH_2, 1.0f);
    led_driver_1->Intensity(KTD2026::Channel::CH_3, 1.0f);
}

void Test_thread::Spectrophotometer_test(I2C_bus &i2c){
    KTD2026 *led_driver_0 = new KTD2026(i2c, 0x31);
    led_driver_0->Init();

    KTD2026 *led_driver_1 = new KTD2026(i2c, 0x30);
    led_driver_1->Init();

    VEML6040 *rgb_sensor = new VEML6040(i2c, 0x10);

    led_driver_0->Intensity(KTD2026::Channel::CH_1, 1.0f);
    led_driver_0->Intensity(KTD2026::Channel::CH_2, 1.0f);
    led_driver_0->Intensity(KTD2026::Channel::CH_3, 1.0f);

    led_driver_1->Intensity(KTD2026::Channel::CH_1, 1.0f);
    led_driver_1->Intensity(KTD2026::Channel::CH_2, 1.0f);
    led_driver_1->Intensity(KTD2026::Channel::CH_3, 1.0f);

    rgb_sensor->Exposure_time(VEML6040::Exposure::_320_ms);
    rtos::Delay(500);

    Logger::Print(emio::format("Blue: {:05d} ", rgb_sensor->Measure(VEML6040::Channels::Blue)));
    Logger::Print(emio::format("Blue: {:05.1f} lux", rgb_sensor->Measure_lux(VEML6040::Channels::Blue)));

    rgb_sensor->Exposure_time(VEML6040::Exposure::_160_ms);
    rtos::Delay(500);
    Logger::Print(emio::format("Blue: {:05d} ", rgb_sensor->Measure(VEML6040::Channels::Blue)));
    Logger::Print(emio::format("Blue: {:05.1f} lux", rgb_sensor->Measure_lux(VEML6040::Channels::Blue)));

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(5000));
        for(float intensity = 0.0f; intensity <= 1.0 ;intensity += 0.05){
            led_driver_0->Intensity(KTD2026::Channel::CH_1, intensity);
            led_driver_0->Intensity(KTD2026::Channel::CH_2, intensity);
            led_driver_0->Intensity(KTD2026::Channel::CH_3, intensity);

            led_driver_1->Intensity(KTD2026::Channel::CH_1, intensity);
            led_driver_1->Intensity(KTD2026::Channel::CH_2, intensity);
            led_driver_1->Intensity(KTD2026::Channel::CH_3, intensity);

            rtos::Delay(200);

            uint16_t det_red   = rgb_sensor->Measure(VEML6040::Channels::Red);
            uint16_t det_green = rgb_sensor->Measure(VEML6040::Channels::Green);
            uint16_t det_blue  = rgb_sensor->Measure(VEML6040::Channels::Blue);
            uint16_t det_white = rgb_sensor->Measure(VEML6040::Channels::White);

            Logger::Notice(emio::format("RGBW: {:6d} {:6d} {:6d} {:6d}", det_red, det_green, det_blue, det_white));

        }
        led_driver_0->Intensity(KTD2026::Channel::CH_1, 0.0f);
        led_driver_0->Intensity(KTD2026::Channel::CH_2, 0.0f);
        led_driver_0->Intensity(KTD2026::Channel::CH_3, 0.0f);

        led_driver_1->Intensity(KTD2026::Channel::CH_1, 0.0f);
        led_driver_1->Intensity(KTD2026::Channel::CH_2, 0.0f);
        led_driver_1->Intensity(KTD2026::Channel::CH_3, 0.0f);
    }

}

void Test_thread::Gain_detector_test(){
    GPIO *es_gain = new GPIO(21, GPIO::Direction::Out);
    es_gain->Set_pulls(false, false);
    es_gain->Set(true);

    GPIO *ts_gain = new GPIO(9, GPIO::Direction::Out);
    ts_gain->Set_pulls(false, false);
    ts_gain->Set(true);

    auto es_det = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_1, 3.30f);
    auto ts_det = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_2, 3.30f);

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1000));

        Logger::Print(emio::format("ES_VOLT: {:05.3f}V", es_det->Read_voltage()));
        Logger::Print(emio::format("TS_VOLT: {:05.3f}V", ts_det->Read_voltage()));
        Logger::Print("-------");
    }
}

void Test_thread::Transmissive_IR_test(I2C_bus &i2c){
    KTD2026 *led_driver_1 = new KTD2026(i2c, 0x30);
    led_driver_1->Init();
    led_driver_1->Intensity(KTD2026::Channel::CH_3, 0.2f);


    GPIO *ts_gain = new GPIO(9, GPIO::Direction::In);
    ts_gain->Set_pulls(false, false);

    auto ts_det = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_2, 3.30f);

    //Logger::Print(emio::format("Gain: {:3d}", 1));
    Logger::Print_raw("\r\n\r\n");

    for (float intensity = 0.0f; intensity <= 1.0f; intensity+= 0.1f) {
        led_driver_1->Intensity(KTD2026::Channel::CH_3, 0.2f);
        rtos::Delay(50);
        //Logger::Print(emio::format("Intensity: {:3d}, TS_VOLT: {:05.3f}V", intensity, ts_det->Read_voltage()));
        Logger::Print_raw(emio::format("{:3.1f}\t{:05.3f}\r\n", intensity, ts_det->Read_voltage()/3.3f));
    }

    ts_gain->Set_direction(GPIO::Direction::Out);
    ts_gain->Set(false);

    //Logger::Print(emio::format("Gain: {:3d}", 10));
    Logger::Print_raw("\r\n\r\n");

    for (float intensity = 0.0f; intensity <= 1.0f; intensity+= 0.1f) {
        led_driver_1->Intensity(KTD2026::Channel::CH_3, 0.2f);
        rtos::Delay(50);
        //Logger::Print(emio::format("Intensity: {:3d}, TS_VOLT: {:05.3f}V", intensity, ts_det->Read_voltage()));
        Logger::Print_raw(emio::format("{:3.1f}\t{:05.3f}\r\n", intensity, ts_det->Read_voltage()/3.3f));
    }

    ts_gain->Set(true);

    //Logger::Print(emio::format("Gain: {:3d}", 50));
    Logger::Print_raw("\r\n\r\n");

    for (float intensity = 0.0f; intensity <= 1.0f; intensity+= 0.1f) {
        led_driver_1->Intensity(KTD2026::Channel::CH_3, 0.2f);
        rtos::Delay(50);
        //Logger::Print(emio::format("Intensity: {:3d}, TS_VOLT: {:05.3f}V", intensity, ts_det->Read_voltage()));
        Logger::Print_raw(emio::format("{:3.1f}\t{:05.3f}\r\n", intensity, ts_det->Read_voltage()/3.3f));
    }

    while(true){
        DelayUntil(fra::Ticks::MsToTicks(1000));
    }

}

void Test_thread::Multi_OJIP(){
    GPIO *es_gain = new GPIO(21, GPIO::Direction::Out);
    es_gain->Set_pulls(true, true);
    es_gain->Set(true);

    // Compute logarithmic intervals
    Sample_timing_generator(SAMPLE_COUNT, 0.5);

    rtos::Delay(500);
    Fluoro_sampler_test();
    rtos::Delay(500);


    es_gain->Set(false);

    rtos::Delay(500);
    Fluoro_sampler_test();
    rtos::Delay(500);

    es_gain->Set_direction(GPIO::Direction::In);

    rtos::Delay(500);
    Fluoro_sampler_test();
    rtos::Delay(500);
}

void Test_thread::Sample_timing_generator(uint32_t sample_count, float total_duration){
    // Calculate the maximum exponent for logarithmic spacing
    double max_exponent = log10(total_duration * 1e6);

    std::vector<float> timings(sample_count,0.0f);

    // Generate sampling times in microseconds
    timings[0] = 2.0; // Starting from 1.0 us
    for (unsigned int i = 1; i < sample_count; ++i) {
        double exponent = (i * max_exponent) / (sample_count - 1);
        double current_time = pow(10, exponent);

        // Apply the minimum time gap between samples
        if ((current_time - timings[i - 1]) < minimal_time_us) {
            double adjusted_time = timings[i - 1] + minimal_time_us;
            timings[i] = pow(10, log10(adjusted_time));
        } else {
            timings[i] = current_time;
        }
    }

    sample_intervals[0] = 2;
    for (unsigned int i = 1; i < sample_count; ++i) {
        sample_intervals[i] = static_cast<uint32_t>(timings[i]-timings[i-1]);
    }

    for (unsigned int i = 0; i < sample_count; ++i) {
        //Logger::Print(emio::format("Timing: {:10.1f} {:10d}", timings[i] ,sample_intervals[i]));
    }
}

void Test_thread::Fluoro_sampler_test(){
    // Initialize ADC
    const uint adc_input_channel = 1;
    adc_init();
    adc_gpio_init(26 + adc_input_channel);
    adc_select_input(adc_input_channel);
    adc_set_clkdiv(0);                          // 0.5 Msps
    adc_fifo_setup(true, true, 1, false, false); // Enable FIFO, 1 sample threshold
    adc_run(true);

    Logger::Print_raw("\r\n\r\n");

    int timestamp_dma_channel = dma_claim_unused_channel(true);
    int wrap_dma_channel      = dma_claim_unused_channel(true);
    int adc_dma_channel       = dma_claim_unused_channel(true);

    const int samples = SAMPLE_COUNT;
    const int slice   = 2;

    pwm_config pwm_cfg = pwm_get_default_config();
    pwm_set_counter(slice, 0);
    pwm_config_set_clkdiv(&pwm_cfg, 125.0);
    pwm_init(slice, &pwm_cfg, false);
    pwm_set_wrap(slice, sample_intervals[0]);

    dma_channel_config timestamp_dma_config = dma_channel_get_default_config(timestamp_dma_channel);
    dma_channel_config wrap_dma_config      = dma_channel_get_default_config(wrap_dma_channel);
    dma_channel_config adc_dma_config       = dma_channel_get_default_config(adc_dma_channel);

    // Configure DMA for 32-bit transfers
    channel_config_set_transfer_data_size(&timestamp_dma_config, DMA_SIZE_32); // 32-bit transfers
    channel_config_set_read_increment(&timestamp_dma_config, false);           // Fixed source address
    channel_config_set_write_increment(&timestamp_dma_config, true);           // Increment destination
    channel_config_set_dreq(&timestamp_dma_config, pwm_get_dreq(slice));            // Trigger DMA by PWM wrap

    channel_config_set_transfer_data_size(&wrap_dma_config, DMA_SIZE_16);      // 16-bit transfers
    channel_config_set_read_increment(&wrap_dma_config, true);                 // Fixed source address
    channel_config_set_write_increment(&wrap_dma_config, false);               // Increment destination
    channel_config_set_dreq(&wrap_dma_config, pwm_get_dreq(slice));                 // Trigger DMA by PWM wrap

    channel_config_set_transfer_data_size(&adc_dma_config, DMA_SIZE_16);       // 16-bit transfers
    channel_config_set_read_increment(&adc_dma_config, false);                 // Fixed source address
    channel_config_set_write_increment(&adc_dma_config, true);                 // Increment destination
    channel_config_set_dreq(&adc_dma_config, pwm_get_dreq(slice));                  // Trigger DMA by PWM wrap

    dma_channel_configure(
        timestamp_dma_channel,
        &timestamp_dma_config,
        timestamp_buffer,    // Destination buffer
        &timer_hw->timerawl, // Source: Timer counter (lower 32 bits), increments every 1 us
        samples,             // Number of transfers
        true                 // Start immediately
    );

    dma_channel_configure(
        wrap_dma_channel,
        &wrap_dma_config,
        &pwm_hw->slice[slice].top, // Destination buffer
        sample_intervals,                // Source: Timer counter (lower 32 bits)
        samples,                   // Number of transfers
        true                       // Start immediately
    );

    dma_channel_configure(
        adc_dma_channel,
        &adc_dma_config,
        adc_buffer,    // Destination buffer
        &adc_hw->fifo, // Source: ADC fifo
        samples,       // Number of transfers
        true           // Start immediately
    );

    uint64_t start_time = to_us_since_boot(get_absolute_time());

    // GPIO *es_gain = new GPIO(21, GPIO::Direction::Out);
    // es_gain->Set_pulls(false, false);
    // es_gain->Set(false);

    const bool boost = false;

    auto led_pwm = new PWM_channel(17, 10000000, 0.0, true);
    //GPIO *led_pwm  = new GPIO(17, GPIO::Direction::Out);

    GPIO *LED_en  = new GPIO(22, GPIO::Direction::Out);
    GPIO *LED_pwm = new GPIO(25, GPIO::Direction::Out);

    if (not boost) {
        led_pwm->Duty_cycle(0.3);
        //led_pwm->Set(true);
    } else {
        LED_en->Set(true);
        LED_pwm->Set(true);
    }

    // Start timer generating request
    pwm_set_enabled(slice, true);

    // Wait until conversion is done
    while (dma_channel_is_busy(timestamp_dma_channel)) {
        rtos::Delay(10);
    }

    if (not boost) {
        led_pwm->Duty_cycle(0.0);
        led_pwm->Stop();
        //led_pwm->Set(false);
    } else {
        LED_pwm->Set(false);
        LED_en->Set(false);
    }

    // Stop timer
    pwm_set_enabled(slice, false);

    uint64_t stop_time = to_us_since_boot(get_absolute_time());
    uint64_t duration  = stop_time - start_time;

    // Logger::Print(emio::format("Start time: {:d} us", start_time));
    // Logger::Print(emio::format("Stop time: {:d} us", stop_time));
    // Logger::Print(emio::format("Duration: {:d} us", duration));

    uint32_t start_timestamp = timestamp_buffer[0] - 3;
    uint32_t start_offset_us    = 20;

    // Print captured data
    for (size_t i = 0; i < samples; i++) {
        //Logger::Print(emio::format("Sample {:3d}: {:6d}, {:4d}", i, timestamp_buffer[i] - start_timestamp, adc_buffer[i]));
        timestamp_buffer[i] = timestamp_buffer[i] - start_timestamp;
        if(timestamp_buffer[i] < start_offset_us){
            timestamp_buffer[i] = 0;
        } else {
            timestamp_buffer[i] -= start_offset_us;
        }
        Logger::Print_raw(emio::format("{:8.6f} \t{:4.3f}\r\n",static_cast<float>(timestamp_buffer[i]), adc_buffer[i]/4095.0f));
    }

    dma_channel_abort(timestamp_dma_channel);
    dma_channel_abort(wrap_dma_channel);
    dma_channel_abort(adc_dma_channel);

    dma_channel_unclaim(timestamp_dma_channel);
    dma_channel_unclaim(wrap_dma_channel);
    dma_channel_unclaim(adc_dma_channel);

    adc_run(false);
}                                                     // Test_thread::Fluoro_sampler_test

void Test_thread::Pacing_timestamp_fast_test(){
    int dma_channel = dma_claim_unused_channel(true); // Claim a DMA channel
    int timer       = dma_claim_unused_timer(true);
    uint dreq       = dma_get_timer_dreq(timer);

    uint32_t interval_us = 100;                             // Interval in microseconds

    uint32_t clock_freq = clock_get_hz(clk_sys);            // Get system clock frequency (Hz)
    // Calculate the interval in clock cycles (system clock ticks)
    uint32_t cycles = (clock_freq * interval_us) / 1000000; // Convert microseconds to cycles

    uint32_t numerator   = 1;
    uint32_t denominator = 12500;

    // Set the timer using dma_timer_set_fraction (ensuring it fits in uint16_t)
    dma_timer_set_fraction(timer, (uint16_t) numerator, (uint16_t) denominator);

    Logger::Print(emio::format("Clock frequency: {:d}", clock_freq));
    Logger::Print(emio::format("Cycle period: {:d}", cycles));
    Logger::Print(emio::format("Numerator: {:d}", numerator));
    Logger::Print(emio::format("Denominator: {:d}", denominator));

    dma_channel_config config = dma_channel_get_default_config(dma_channel);

    // Configure DMA for 32-bit transfers
    channel_config_set_transfer_data_size(&config, DMA_SIZE_32); // 32-bit transfers
    channel_config_set_read_increment(&config, false);           // Fixed source address
    channel_config_set_write_increment(&config, true);           // Increment destination

    // Use the pacing timer for DMA requests
    channel_config_set_dreq(&config, dreq);
    // dma_channel_set_trigger(dma_adc_channel, DREQ_DMA_TIMER0);

    uint64_t start_time = to_us_since_boot(get_absolute_time());

    // Configure the DMA channel
    dma_channel_configure(
        dma_channel,
        &config,
        timestamp_buffer,    // Destination buffer
        &timer_hw->timerawl, // Source: Timer counter (lower 32 bits)
        BUFFER_SIZE,         // Number of transfers
        true                 // Start immediately
    );

    while (dma_channel_is_busy(dma_channel)) {
        // Busy-wait until the transfer completes
        // You can optionally add a small delay here if desired
    }

    uint64_t stop_time = to_us_since_boot(get_absolute_time());
    uint64_t duration  = stop_time - start_time;

    Logger::Print(emio::format("Start time: {:d} us", start_time));
    Logger::Print(emio::format("Stop time: {:d} us", stop_time));
    Logger::Print(emio::format("Duration: {:d} us", duration));

    // Print captured data
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        Logger::Print(emio::format("Sample {:3d}: {:5d}", i, timestamp_buffer[i]));
    }
}                                                     // Test_thread::Pacing_timestamp_fast_test

void Test_thread::Pacing_timestamp_slow_test(){
    int dma_channel = dma_claim_unused_channel(true); // Claim a DMA channel

    int slice = 0;

    pwm_config pwm_cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&pwm_cfg, 1.250); // Fastest clock
    pwm_init(slice, &pwm_cfg, false);
    pwm_set_wrap(slice, 1000);              // Set initial delay
    pwm_set_enabled(slice, true);

    dma_channel_config config = dma_channel_get_default_config(dma_channel);

    // Configure DMA for 32-bit transfers
    channel_config_set_transfer_data_size(&config, DMA_SIZE_32); // 32-bit transfers
    channel_config_set_read_increment(&config, false);           // Fixed source address
    channel_config_set_write_increment(&config, true);           // Increment destination
    channel_config_set_dreq(&config, DREQ_PWM_WRAP0);            // Trigger DMA by PWM wrap

    uint64_t start_time = to_us_since_boot(get_absolute_time());

    // Configure the DMA channel
    dma_channel_configure(
        dma_channel,
        &config,
        timestamp_buffer,    // Destination buffer
        &timer_hw->timerawl, // Source: Timer counter (lower 32 bits)
        BUFFER_SIZE,         // Number of transfers
        true                 // Start immediately
    );

    while (dma_channel_is_busy(dma_channel)) {
        // Busy-wait until the transfer completes
        // You can optionally add a small delay here if desired
    }

    uint64_t stop_time = to_us_since_boot(get_absolute_time());
    uint64_t duration  = stop_time - start_time;

    Logger::Print(emio::format("Start time: {:d} us", start_time));
    Logger::Print(emio::format("Stop time: {:d} us", stop_time));
    Logger::Print(emio::format("Duration: {:d} us", duration));

    // Print captured data
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        Logger::Print(emio::format("Sample {:3d}: {:5d}", i, timestamp_buffer[i]));
    }
}                                                               // Test_thread::Pacing_timestamp_slow_test

void Test_thread::Pacing_timestamp_nonlinear_test(){
    int dma_timestamp_channel = dma_claim_unused_channel(true); // Claim a DMA channel
    int dma_wrap_channel      = dma_claim_unused_channel(true); // Claim a DMA channel

    const int samples = 10;

    const int slice = 0;

    pwm_config pwm_cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&pwm_cfg, 1.250); // Fastest clock
    pwm_init(slice, &pwm_cfg, false);
    pwm_set_wrap(slice, 1000);              // Set initial delay


    dma_channel_config timestamp_dma_config = dma_channel_get_default_config(dma_timestamp_channel);
    dma_channel_config wrap_dma_config      = dma_channel_get_default_config(dma_wrap_channel);

    // Configure DMA for 32-bit transfers
    channel_config_set_transfer_data_size(&timestamp_dma_config, DMA_SIZE_32); // 32-bit transfers
    channel_config_set_read_increment(&timestamp_dma_config, false);           // Fixed source address
    channel_config_set_write_increment(&timestamp_dma_config, true);           // Increment destination
    channel_config_set_dreq(&timestamp_dma_config, DREQ_PWM_WRAP0);            // Trigger DMA by PWM wrap

    channel_config_set_transfer_data_size(&wrap_dma_config, DMA_SIZE_32);      // 32-bit transfers
    channel_config_set_read_increment(&wrap_dma_config, true);                 // Fixed source address
    channel_config_set_write_increment(&wrap_dma_config, false);               // Increment destination
    channel_config_set_dreq(&wrap_dma_config, DREQ_PWM_WRAP0);                 // Trigger DMA by PWM wrap

    dma_channel_configure(
        dma_timestamp_channel,
        &timestamp_dma_config,
        timestamp_buffer,    // Destination buffer
        &timer_hw->timerawl, // Source: Timer counter (lower 32 bits)
        samples,             // Number of transfers
        true                 // Start immediately
    );

    // Configure the DMA channel
    dma_channel_configure(
        dma_wrap_channel,
        &wrap_dma_config,
        &pwm_hw->slice[slice].top, // Destination buffer
        timing,                    // Source: Timer counter (lower 32 bits)
        samples,                   // Number of transfers
        true                       // Start immediately
    );

    uint64_t start_time = to_us_since_boot(get_absolute_time());

    pwm_set_enabled(slice, true);

    while (dma_channel_is_busy(dma_timestamp_channel)) {
        // Busy-wait until the transfer completes
        // You can optionally add a small delay here if desired
    }

    uint64_t stop_time = to_us_since_boot(get_absolute_time());
    uint64_t duration  = stop_time - start_time;

    Logger::Print(emio::format("Start time: {:d} us", start_time));
    Logger::Print(emio::format("Stop time: {:d} us", stop_time));
    Logger::Print(emio::format("Duration: {:d} us", duration));

    // Print captured data
    for (size_t i = 0; i < samples; i++) {
        Logger::Print(emio::format("Sample {:3d}: {:5d}", i, timestamp_buffer[i]));
    }
}  // Test_thread::Pacing_timestamp_nonlinear_test
