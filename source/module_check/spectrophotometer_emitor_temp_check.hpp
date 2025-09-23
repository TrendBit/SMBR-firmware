#pragma once
#include "module_check/limit_check.hpp"
#include "components/spectrophotometer.hpp"
#include "modules/base_module.hpp"

class Spectrophotometer_emitor_temp_check : public Limit_check<Spectrophotometer> {
public:
    explicit Spectrophotometer_emitor_temp_check(Spectrophotometer* spectro)
        : Limit_check<Spectrophotometer>(
            spectro,
            [](Spectrophotometer* s){ return std::optional<float>(s->Temperature()); },
            70.0f,
            App_messages::Module_issue::IssueType::SpectrophotometerEmitorOverTemp,
            "Spectrophotometer_emitor_temp_check",
            [spectro](const App_messages::Module_issue::Module_issue& issue) {
                auto copy = issue;
                spectro->Send_CAN_message(copy);
            })
    {}
};
