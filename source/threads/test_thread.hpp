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

#include "thread.hpp"

namespace fra = cpp_freertos;

class Test_thread : public fra::Thread {
private:
    CAN_thread * can_thread;

public:
    explicit Test_thread(CAN_thread * can_thread);

    Test_thread();

protected:
    virtual void Run();
<<<<<<< HEAD

    void Test_RPM();

    void Test_heater();

    void Test_temps();

    void Test_motors();
||||||| parent of 65d1efd (test_thread: add testing of sensor_module components)
=======

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
>>>>>>> 65d1efd (test_thread: add testing of sensor_module components)
};

