/**
 * @file sensor_module.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 22.08.2024
 */

#include <limits>

#include "base_module.hpp"
#include "codes/codes.hpp"
#include "logger.hpp"

#include "components/mini_oled.hpp"

/**
 * @brief Sensor module in measuring compartment of device, enables:
 *          - Optical density of the suspension
 *          -
 *
 */
class Sensor_module: public Base_module {
private:
    /**
     * @brief   Mini OLED display component
     */
    Mini_OLED * mini_oled;

public:
    /**
     * @brief Construct a new Sensor_module object, calls constructor of Base_module with type of module
     *          This module should be exclusive in system (Exclusive instance)
     */
    Sensor_module();

    /**
     * @brief Perform setup of all components of module
     */
    virtual void Setup_components() override final;

    /**
     * @brief   Retrieves current temperature of board from onboard thermistor, which is behind multiplexor with led thermistor
     *
     * @return float    Temperature of board in Celsius
     */
    virtual float Board_temperature() override;

private:
    /**
     * @brief   Setup Mini OLED display
     */
    void Setup_Mini_OLED();
};
