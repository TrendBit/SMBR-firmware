/**
 * @file pump_module.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 26.08.2025
 */

#pragma once

#include "base_module.hpp"
#include "codes/codes.hpp"
#include "logger.hpp"

#include "components/pumps.hpp"

/**
 * @brief   Module responsible for control of additional pumps
 *          Module can be configure for 2 or 4 pumps based on configuration pin state
 *          This module can have multiple instances in system
 */
class Pump_module: public Base_module {
private:
    /**
     * @brief   Thermistor to measure onboard temperature
     */
    Thermistor * const board_thermistor;

    /**
     * @brief   GPIO of MCU which is connected to enumeration button
     */
    static constexpr uint enumeration_button_gpio = 22;

    /**
     * @brief   GPIO of MCU which is connected to enumeration RGB LED
     */
    static constexpr uint enumeration_rgb_led_gpio = 23;

    /**
     * @brief   GPIO of MCU which is connected to activity green LED
     */
    static constexpr uint activity_green_led_gpio = 24;

    /**
     * @brief   GPIO of MCU which is connected to I2C SDA line
     */
    static constexpr uint i2c_sda_gpio = 18;

    /**
     * @brief   GPIO of MCU which is connected to I2C SCL line
     */
    static constexpr uint i2c_scl_gpio = 19;

    /**
     * @brief   GPIO of MCU which is connected to pump configuration pin
     *          Based on value of this pin number of pumps is set up.
     */
    static const uint config_detection_pin = 25;

    /**
     * @brief   Component responsible for control of pumps connected to module
     */
    Pump_controller * pump_controller;

public:
    /**
     * @brief Construct a new Pump module object, this is Enumerative module
     */
    Pump_module();

    /**
     * @brief Perform setup of all components of module
     */
    virtual void Setup_components() override final;

    /**
     * @brief   Retrieves current temperature of board from onboard thermistor
     *
     * @return float    Temperature of board in Celsius
     */
    virtual std::optional<float> Board_temperature() override;
};
