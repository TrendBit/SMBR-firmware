#pragma once

#include "logger.hpp"
#include "module_check/IModuleCheck.hpp"
#include "components/fluorometer.hpp"
#include "components/component.hpp"
#include "modules/base_module.hpp"

/**
 * @brief Fluorometer detector temperature check
 */
class Fluorometer_detector_temp_check : public IModuleCheck {
public:
    explicit Fluorometer_detector_temp_check(Fluorometer* fluorometer);

    void Run_check() override;

private:
    Fluorometer* fluorometer;
};
