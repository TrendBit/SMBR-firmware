/**
 * @file control_module.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 14.08.2024
 */

#pragma once

#include "base_module.hpp"
#include "logger.hpp"
#include "codes/codes.hpp"

#include "threads/test_thread.hpp"

#include "components/led/led_pwm.hpp"
#include "components/led_panel.hpp"
#include "components/heater.hpp"
#include "components/cuvette_pump.hpp"

/**
 * @brief Control module shield used for:
 *          - Temperature control
 *          - Fan control (chassis, temp fan, mixing fan)
 *          - Mixing of suspense in bottle
 *          - LED panel control
 *          - Pumping of suspense in cuvette of sensor module
 *          - Aerating of bottle
 *          - Additional temperature sensing (for example LED panel temperature)
 *        Exclusive module (only one instance in system)
 */
class Control_module: public Base_module {
private:
    /**
     * @brief   LED panel component
     */
    LED_panel * led_panel = nullptr;

    /**
     * @brief   Heater component
     */
    Heater * heater = nullptr;

    /**
     * @brief   Cuvette pump component
     */
    Cuvette_pump * cuvette_pump = nullptr;

    /**
     * @brief   Thermistor to measure onboard temperature
     */
    Thermistor * const board_thermistor;

public:
    /**
     * @brief Construct a new Control_module object, calls constructor of Base_module with type of module
     *          This module should be exclusive in system (Exclusive instance)
     */
    Control_module();

    /**
     * @brief Perform setup of all components of module
     */
    virtual void Setup_components() override final;

private:
    /**
     * @brief   Configure LED panel with all channels and temperature sensor
     */
    void Setup_LEDs();

    /**
     * @brief   Configure heater for temperature control
     */
    void Setup_heater();

    /**
     * @brief   Configure peristaltic pump for moving liquid into cuvette
     */
    void Setup_cuvette_pump();

    /**
     * @brief   Retrieves current temperature of board from onboard thermistor
     *
     * @return float    Temperature of board in Celsius
     */
    virtual float Board_temperature() override;

};
