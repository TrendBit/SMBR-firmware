/**
 * @file test_thread.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 02.06.2024
 */

#pragma once

#include "can_bus/can_bus.hpp"
#include "threads/can_thread.hpp"
#include "hal/gpio/gpio.hpp"
#include "hal/pwm/pwm.hpp"
#include "components/thermometers/thermistor.hpp"
#include "hal/adc/adc_channel.hpp"
#include "components/common_core.hpp"

#include "components/common_sensors/rpm_counter_pio.hpp"
#include "threads/mini_display_thread.hpp"

#include "hardware/dma.h"
#include "hardware/clocks.h"

#include "thread.hpp"

#define SAMPLE_COUNT 200     // Number of intervals (samples)
#define TOTAL_DURATION_US 2000000 // Total sampling duration in microseconds

#define BUFFER_SIZE 100

namespace fra = cpp_freertos;

// DMA channels
inline int dma_adc_channel, dma_timer_channel;

class Test_thread : public fra::Thread {
private:
    CAN_thread * can_thread;

    uint16_t sample_intervals[SAMPLE_COUNT]; // Logarithmic delay intervals
    uint32_t timestamps[SAMPLE_COUNT]; // Logarithmic delay intervals
    uint16_t adc_buffer[SAMPLE_COUNT];   // ADC data buffer

    uint32_t timing[10] = {1000, 1000, 1000, 10000, 10000, 10000, 10000, 10000, 10000, 10000};

    volatile uint32_t timestamp_buffer[BUFFER_SIZE];

    float minimal_time_us = 2.0;

public:
    explicit Test_thread(CAN_thread * can_thread);

    Test_thread();

protected:
    virtual void Run();

    void Test_RPM();

    void Test_heater();

    void Test_temps();

    void Test_motors();

private:
    void EEPROM_Test(I2C_bus &i2c);

    void Thermopile_test(I2C_bus &i2c);

    void Fluoro_buck_test();

    void Fluoro_boost_test();

    void Temp_sensor_test(I2C_bus &i2c);

    void RGBW_sensor_test(I2C_bus &i2c);

    void LED_test(I2C_bus &i2c);

    void Gain_detector_test();

    void OLED_test();

    void Sample_timing_generator(uint32_t count = SAMPLE_COUNT , float total_duration = 2.0);

    void Fluoro_sampler_test();

    void Pacing_timestamp_fast_test();

    void Pacing_timestamp_slow_test();

    void Pacing_timestamp_nonlinear_test();

    void Transmissive_IR_test(I2C_bus &i2c);

    void Multi_OJIP();

};

