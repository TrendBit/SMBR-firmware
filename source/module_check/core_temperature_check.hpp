#pragma once
#include "module_check/limit_check.hpp"
#include "components/common_core.hpp"

class Core_temperature_check : public Limit_check<Common_core> {
public:
    explicit Core_temperature_check(Common_core* core)
        : Limit_check<Common_core>(
            core,
            [](Common_core* c){ return c->MCU_core_temperature(); },
            70.0f,
            App_messages::Module_issue::IssueType::CoreOverTemp,
            "Core_temperature_check",
            [core](const App_messages::Module_issue::Module_issue& issue) {
                auto copy = issue;  
                core->Send_CAN_message(copy);
            })
    {}
};
