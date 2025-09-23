#pragma once
#include "module_check/limit_check.hpp"
#include "components/fluorometer.hpp"

class Fluorometer_detector_temp_check : public Limit_check<Fluorometer> {
public:
    explicit Fluorometer_detector_temp_check(Fluorometer* fluorometer)
        : Limit_check<Fluorometer>(
            fluorometer,
            [](Fluorometer* f){ return f->Detector_temperature(); },
            70.0f,
            App_messages::Module_issue::IssueType::FluorometerDetectorOverTemp,
            "Fluorometer_detector_temp_check",
            [fluorometer](const App_messages::Module_issue::Module_issue& issue) {
                auto copy = issue;  
                fluorometer->Send_CAN_message(copy);
            })
    {}
};
