#pragma once

#include "logger.hpp"
#include "module_check/IModuleCheck.hpp"
#include "components/spectrophotometer.hpp"
#include "components/component.hpp"
#include "modules/base_module.hpp"

/**
 * @brief Spectrophotometer emitor temperature check
 */
class Spectrophotometer_emitor_temp_check : public IModuleCheck {
public:
    explicit Spectrophotometer_emitor_temp_check(Spectrophotometer* spectrophotometer);

    void Run_check() override;

private:
    Spectrophotometer* spectrophotometer;
};
