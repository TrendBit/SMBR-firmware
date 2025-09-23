#pragma once
#include "module_check/limit_check.hpp"
#include "components/common_core.hpp"

class Core_load_check : public Limit_check<Common_core> {
public:
    explicit Core_load_check(Common_core* core)
        : Limit_check<Common_core>(
            core,
            [](Common_core* c){ return c->Get_core_load(); },
            0.9f,
            App_messages::Module_issue::IssueType::HighLoad,
            "Core_load_check",
            [core](const App_messages::Module_issue::Module_issue& issue) {
                auto copy = issue;  
                core->Send_CAN_message(copy);
            })
    {}
};
