#include "module_check/core_temperature_check.hpp"
#include "codes/messages/module_issues/module_issue.hpp"
#include "logger.hpp"

Core_temperature_check::Core_temperature_check(Common_core* core)
    : core(core)
{}

void Core_temperature_check::Run_check() {
    if (core == nullptr) {
        Logger::Error("Core_temperature_check: core is nullptr");
        return;
    }

    auto temp = core->MCU_core_temperature();

    if (!temp.has_value()) {
        Logger::Warning("Core_temperature_check: temperature not available");
        return;
    }

    float value = temp.value();

    if (value > 70.0f) {  
        Logger::Warning("Core_temperature_check: High temperature detected ({:.2f} deg C)", value);

        App_messages::Module_issue::Module_issue issue(
            App_messages::Module_issue::IssueType::CoreOverTemp,
            App_messages::Module_issue::Severity::Error,
            value
        );

        core->Send_CAN_message(issue);
    }
}