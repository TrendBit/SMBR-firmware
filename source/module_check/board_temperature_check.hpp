#pragma once
#include "module_check/limit_check.hpp"
#include "modules/base_module.hpp"

class Board_temperature_check : public Limit_check<Base_module> {
public:
    explicit Board_temperature_check(Base_module* module)
        : Limit_check<Base_module>(
            module,
            [](Base_module* m){ return m->Board_temperature(); },
            70.0f,
            App_messages::Module_issue::IssueType::BoardOverTemp,
            "Board_temperature_check",
            [module](const App_messages::Module_issue::Module_issue& issue) {
                auto copy = issue;  
                module->Send_CAN_message(copy);
            })
    {}
};
