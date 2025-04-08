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
#include "components/adc/TLA2024.hpp"
#include "components/adc/TLA2024_channel.hpp"
#include "components/thermometers/thermopile.hpp"
#include "components/bottle_temperature.hpp"
#include "components/fluorometer.hpp"
#include "components/spectrophotometer.hpp"

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

    /**
     * @brief   Bottle temperature measuring sensor unit component
     */
    Bottle_temperature * bottle_temperature;

    /**
     * @brief   GPIO controlling input of multiplexor for temperature ADC measurement
     *              1 - onboard thermistor
     *              0 - Fluoro LED thermistor
     */
    GPIO * const ntc_channel_selector;

    /**
     * @brief   ADC channel for measuring temperature of onboard thermistor or Fluoro LED thermistor
     *          Thermistor have same values of nominal resistance and B value
     */
    Thermistor * const ntc_thermistors;

    /**
     * @brief  Fluorometer components used for OJIP measurements
     */
    Fluorometer * fluorometer;

    /**
     * @brief  Multi spectral photo meter used for measuring optical density of suspension
     */
    Spectrophotometer * spectrophotometer;

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

    /**
     * @brief   Setup and configure thermopile sensors for measuring temperature of bottle
     */
    void Setup_bottle_thermometers();

    /**
     * @brief   Setup fluorometer for OJIP measurements
     */
    void Setup_fluorometer();

    /**
     * @brief   Setup spectrophotometer for measuring optical density of suspension
     */
    void Setup_spectrophotometer();
};
