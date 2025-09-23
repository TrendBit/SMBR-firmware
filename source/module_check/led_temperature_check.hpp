#pragma once
#include "module_check/limit_check.hpp"
#include "components/led_panel.hpp"
#include "modules/base_module.hpp"

class Led_temperature_check : public Limit_check<LED_panel> {
public:
    explicit Led_temperature_check(LED_panel* panel)
        : Limit_check<LED_panel>(
            panel,
            [](LED_panel* p){ return std::optional<float>(p->Temperature()); },
            70.0f,
            App_messages::Module_issue::IssueType::LEDPanelOverTemp,
            "Led_temperature_check",
            [panel](const App_messages::Module_issue::Module_issue& issue) {
                auto copy = issue;
                panel->Send_CAN_message(copy);
            })
    {}
};
