#include "module_check/heater_plate_temp_check.hpp"
#include "codes/messages/module_issues/module_issue.hpp"

Heater_plate_temp_check::Heater_plate_temp_check(Heater* heater)
    : heater(heater)
{}

void Heater_plate_temp_check::Run_check() {
    if (heater == nullptr) {
        Logger::Error("Heater_plate_temp_check: Heater is nullptr");
        return;
    }

    float temp = heater->Temperature();

    if (std::isnan(temp)) {
        Logger::Warning("Heater_plate_temp_check: sensor not available");
        return;
    }

    if (temp > 80.0f) {  
        Logger::Warning("Heater_plate_temp_check: High temperature detected ({:.2f} deg C)", temp);
        App_messages::Module_issue::Module_issue issue(
            App_messages::Module_issue::IssueType::HeaterOverTemp,
            App_messages::Module_issue::Severity::Error,
            temp
        );

        Base_module::Singleton_instance()->Send_CAN_message(issue);
    }
}
