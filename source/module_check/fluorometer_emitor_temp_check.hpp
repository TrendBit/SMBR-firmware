#pragma once
#include "module_check/limit_check.hpp"
#include "components/fluorometer.hpp"
#include "modules/base_module.hpp"

class Fluorometer_emitor_temp_check : public Limit_check<Fluorometer> {
public:
    explicit Fluorometer_emitor_temp_check(Fluorometer* fluorometer)
        : Limit_check<Fluorometer>(
            fluorometer,
            [](Fluorometer* f){ return f->Emitor_temperature(); },
            70.0f,
            App_messages::Module_issue::IssueType::FluorometerEmitorOverTemp,
            "Fluorometer_emitor_temp_check",
            [fluorometer](const App_messages::Module_issue::Module_issue& issue) {
                auto copy = issue;
                fluorometer->Send_CAN_message(copy);
            })
    {}
};
