#pragma once

#include "logger.hpp"
#include "module_check/IModuleCheck.hpp"
#include "components/heater.hpp"
#include "components/component.hpp"
#include "modules/base_module.hpp"

/**
 * @brief Heater plate temperature check
 */
class Heater_plate_temp_check : public IModuleCheck {
public:
    /**
     * @brief Construct a new Heater_plate_temp_check
     *
     */
    explicit Heater_plate_temp_check(Heater* heater);

    /**
     * @brief Performs the check
     */
    void Run_check() override;

private:
    Heater* heater;
};
