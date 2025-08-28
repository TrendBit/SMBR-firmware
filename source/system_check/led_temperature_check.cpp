#include "system_check/led_temperature_check.hpp"
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
        App_messages::Module_issues::Module_issue issue(
            1, //error_type; e.g. 1 = LED_OVERHEAT
            2, // severity; e.g. 2 = warning, 10 = critical
            temp // value
        );

        Base_module::Instance()->Send_CAN_message(issue); 
    } else {
        // Logger::Debug("Led_temperature_check: Temperature OK ({:.2f} deg C)", temp);
    }
}
