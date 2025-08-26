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

class Pump_module: public Base_module {

    /**
     * @brief   Thermistor to measure onboard temperature
     */
    Thermistor * const board_thermistor;

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
