#include "module_check/fluorometer_emitor_temp_check.hpp"
#include "codes/messages/module_issues/module_issue.hpp"

Fluorometer_emitor_temp_check::Fluorometer_emitor_temp_check(Fluorometer* fluorometer)
    : fluorometer(fluorometer)
{}

void Fluorometer_emitor_temp_check::Run_check() {
    if (!fluorometer) {
        Logger::Error("Fluorometer_emitor_temp_check: Fluorometer is nullptr");
        return;
    }

    auto temp_opt = fluorometer->Emitor_temperature();
    if (!temp_opt.has_value()) {
        Logger::Warning("Fluorometer_emitor_temp_check: sensor not available");
        return;
    }

    float temp = temp_opt.value();

    if (temp > 70.0f) {
        Logger::Warning("Fluorometer_emitor_temp_check: High emitor temp detected ({:.2f} deg C)", temp);
        App_messages::Module_issue::Module_issue issue(
            App_messages::Module_issue::IssueType::FluorometerEmitorOverTemp,
            App_messages::Module_issue::Severity::Error,
            temp
        );

        Base_module::Singleton_instance()->Send_CAN_message(issue);
    }
}
