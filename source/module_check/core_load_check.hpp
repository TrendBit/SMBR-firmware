#pragma once

#include "module_check/IModuleCheck.hpp"
#include "components/common_core.hpp"

/**
 * @brief Periodically checks the MCU core load
 *
 */
class Core_load_check : public IModuleCheck {
public:

    /**
     * @brief Construct a new Core_load_check
     *
     */
    explicit Core_load_check(Common_core* core);

    /**
     * @brief Performs the check
     */
    void Run_check() override;

private:
    Common_core* core;
};