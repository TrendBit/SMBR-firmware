#pragma once

#include "logger.hpp"
#include "module_check/IModuleCheck.hpp"
#include "components/bottle_temperature.hpp"
#include "components/component.hpp"
#include "modules/base_module.hpp"

/**
 * @brief Bottle top measured temperature check
 */
class Bottle_top_measured_temp_check : public IModuleCheck {
public:
    explicit Bottle_top_measured_temp_check(Bottle_temperature* bottle);

    void Run_check() override;

private:
    Bottle_temperature* bottle;
};
