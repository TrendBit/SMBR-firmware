#include "module_check/fluorometer_detector_temp_check.hpp"
#include "codes/messages/module_issues/module_issue.hpp"

Fluorometer_detector_temp_check::Fluorometer_detector_temp_check(Fluorometer* fluorometer)
    : fluorometer(fluorometer)
{}

void Fluorometer_detector_temp_check::Run_check() {
    if (!fluorometer) {
        Logger::Error("Fluorometer_detector_temp_check: Fluorometer is nullptr");
        return;
    }

    float temp = fluorometer->Detector_temperature();

    if (std::isnan(temp)) {
        Logger::Warning("Fluorometer_detector_temp_check: sensor not available");
        return;
    }

    if (temp > 70.0f) { 
        Logger::Warning("Fluorometer_detector_temp_check: High detector temp detected ({:.2f} deg C)", temp);
        App_messages::Module_issue::Module_issue issue(
            App_messages::Module_issue::IssueType::FluorometerDetectorOverTemp,
            App_messages::Module_issue::Severity::Error,
            temp
        );

        Base_module::Singleton_instance()->Send_CAN_message(issue);
    }
}
