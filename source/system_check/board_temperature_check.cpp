#include "system_check/board_temperature_check.hpp"
#include "codes/messages/module_issues/module_issue.hpp"
#include "logger.hpp"
#include <cmath>

Board_temperature_check::Board_temperature_check(Base_module* module)
    : module(module)
{}

void Board_temperature_check::Run_check() {
    if (module == nullptr) {
        Logger::Error("Board_temperature_check: module is nullptr");
        return;
    }

    auto temp = module->Board_temperature();

    if (!temp.has_value()) {
        Logger::Warning("Board_temperature_check: temperature not available");
        return;
    }

    float value = temp.value();

    if (value > 60.0f) {  
        Logger::Warning("Board_temperature_check: High temperature detected ({:.2f} deg C)", value);

        App_messages::Module_issue::Module_issue issue(
            App_messages::Module_issue::IssueType::BoardOverTemp,
            App_messages::Module_issue::Severity::Error,
            value
        );

        module->Send_CAN_message(issue);
    } 
}
