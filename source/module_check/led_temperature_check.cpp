#include "module_check/led_temperature_check.hpp"
#include "codes/messages/module_issues/module_issue.hpp"

Led_temperature_check::Led_temperature_check(LED_panel* panel)
    : panel(panel)
{}

void Led_temperature_check::Run_check() {
    if (panel == nullptr) {
        Logger::Error("Led_temperature_check: LED_panel is nullptr");
        return;
    }

    float temp = panel->Temperature();

    if (std::isnan(temp)) {
        Logger::Warning("Led_temperature_check: sensor not available");
        return;
    }

    if (temp > 70.0f) {
        Logger::Warning("Led_temperature_check: High temperature detected ({:.2f} deg C)", temp);
        App_messages::Module_issue::Module_issue issue(
            App_messages::Module_issue::IssueType::LEDPanelOverTemp, 
            App_messages::Module_issue::Severity::Error, 
            temp 
        );

        Base_module::Instance()->Send_CAN_message(issue); 
    } 
}
