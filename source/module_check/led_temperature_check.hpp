#pragma once

#include "logger.hpp"
#include "module_check/IModuleCheck.hpp"
#include "components/led_panel.hpp"
#include "components/component.hpp"
#include "modules/base_module.hpp"

/**
 * @brief LED panel temperature check
 *
 */
class Led_temperature_check : public IModuleCheck{
public:
    /**
     * @brief Construct a new Led_temperature_check
     *
     */ 
    explicit Led_temperature_check(LED_panel* panel);

    /**
     * @brief Performs the check
     */
    void Run_check() override;

private:
    LED_panel * panel;
};



