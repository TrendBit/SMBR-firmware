#include "module_check/spectrophotometer_emitor_temp_check.hpp"
#include "codes/messages/module_issues/module_issue.hpp"

Spectrophotometer_emitor_temp_check::Spectrophotometer_emitor_temp_check(Spectrophotometer* spectrophotometer)
    : spectrophotometer(spectrophotometer)
{}

void Spectrophotometer_emitor_temp_check::Run_check() {
    if (!spectrophotometer) {
        Logger::Error("Spectrophotometer_emitor_temp_check: Spectrophotometer is nullptr");
        return;
    }

    float temp = spectrophotometer->Temperature();

    if (std::isnan(temp)) {
        Logger::Warning("Spectrophotometer_emitor_temp_check: sensor not available");
        return;
    }

    if (temp > 70.0f) {
        Logger::Warning("Spectrophotometer_emitor_temp_check: High emitor temp detected ({:.2f} deg C)", temp);
        App_messages::Module_issue::Module_issue issue(
            App_messages::Module_issue::IssueType::SpectrophotometerEmitorOverTemp,
            App_messages::Module_issue::Severity::Error,
            temp
        );

        Base_module::Singleton_instance()->Send_CAN_message(issue);
    }
}
