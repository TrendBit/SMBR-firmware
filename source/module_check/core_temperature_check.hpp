#pragma once

#include "module_check/IModuleCheck.hpp"
#include "components/common_core.hpp"

/**
 * @brief Periodically checks the MCU core temperature
 *
 */
class Core_temperature_check : public IModuleCheck {
public:

    /**
     * @brief Construct a new Core_temperature_check
     *
     */
    explicit Core_temperature_check(Common_core* core);

    /**
     * @brief Performs the check
     */
    void Run_check() override;

private:
    Common_core* core;
};