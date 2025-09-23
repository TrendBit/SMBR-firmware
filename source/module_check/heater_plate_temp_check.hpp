#pragma once
#include "module_check/limit_check.hpp"
#include "components/heater.hpp"

class Heater_plate_temp_check : public Limit_check<Heater> {
public:
    explicit Heater_plate_temp_check(Heater* heater)
        : Limit_check<Heater>(
            heater,
            [](Heater* h){ return std::optional<float>(h->Temperature()); },
            80.0f,
            App_messages::Module_issue::IssueType::HeaterOverTemp,
            "Heater_plate_temp_check",
            [heater](const App_messages::Module_issue::Module_issue& issue) {
                auto copy = issue;
                heater->Send_CAN_message(copy);
            })
    {}
};
