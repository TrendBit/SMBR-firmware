#pragma once

#include "logger.hpp"
#include "components/led_panel.hpp"
#include "components/component.hpp"
#include "modules/base_module.hpp"

/**
 * @brief LED Panel Temperature Check
 *
 * If the temperature exceeds 70 °C, it logs a warning.
 */
class Led_temperature_check {
public:
    Led_temperature_check(LED_panel* panel);

    /**
     * @brief Performs the check
     */
    void Run_check();

private:
    LED_panel * const panel;
};
