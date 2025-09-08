#include "module_check/bottle_top_sensor_temp_check.hpp"
#include "codes/messages/module_issues/module_issue.hpp"

Bottle_top_sensor_temp_check::Bottle_top_sensor_temp_check(Bottle_temperature* bottle)
    : bottle(bottle)
{}

void Bottle_top_sensor_temp_check::Run_check() {
    if (!bottle) {
        Logger::Error("Bottle_top_sensor_temp_check: Bottle_temperature is nullptr");
        return;
    }

    float temp = bottle->Top_sensor_temperature();

    if (std::isnan(temp)) {
        Logger::Warning("Bottle_top_sensor_temp_check: sensor not available");
        return;
    }

    if (temp > 70.0f) { 
        Logger::Warning("Bottle_top_sensor_temp_check: High top sensor temp detected ({:.2f} deg C)", temp);
        App_messages::Module_issue::Module_issue issue(
            App_messages::Module_issue::IssueType::BottleTopOverSensorTemp,
            App_messages::Module_issue::Severity::Error,
            temp
        );

        Base_module::Instance()->Send_CAN_message(issue);
    }
}
