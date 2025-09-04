#pragma once

#include "module_check/IModuleCheck.hpp"
#include "modules/base_module.hpp"


/**
 * @brief Periodically checks the board temperature
 *
 */ 
class Board_temperature_check : public IModuleCheck {
public:

    /**
     * @brief Construct a new Board_temperature_check
     *
     */
    explicit Board_temperature_check(Base_module* module);

    /**
     * @brief Performs the check
     */
    void Run_check() override;

private:
    Base_module* module;
};
