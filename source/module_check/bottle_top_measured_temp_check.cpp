#include "module_check/bottle_top_measured_temp_check.hpp"
#include "codes/messages/module_issues/module_issue.hpp"

Bottle_top_measured_temp_check::Bottle_top_measured_temp_check(Bottle_temperature* bottle)
    : bottle(bottle)
{}

void Bottle_top_measured_temp_check::Run_check() {
    if (!bottle) {
        Logger::Error("Bottle_top_measured_temp_check: Bottle_temperature is nullptr");
        return;
    }

    float temp = bottle->Top_temperature();

    if (std::isnan(temp)) {
        Logger::Warning("Bottle_top_measured_temp_check: sensor not available");
        return;
    }

    if (temp > 70.0f) { 
        Logger::Warning("Bottle_top_measured_temp_check: High top temp detected ({:.2f} deg C)", temp);
        App_messages::Module_issue::Module_issue issue(
            App_messages::Module_issue::IssueType::BottleTopOverMeasTemp,
            App_messages::Module_issue::Severity::Error,
            temp
        );

        Base_module::Singleton_instance()->Send_CAN_message(issue);
    }
}
