#pragma once
#include "module_check/limit_check.hpp"
#include "components/bottle_temperature.hpp"

class Bottle_bottom_sensor_temp_check : public Limit_check<Bottle_temperature> {
public:
    explicit Bottle_bottom_sensor_temp_check(Bottle_temperature* bottle)
        : Limit_check<Bottle_temperature>(
            bottle,
            [](Bottle_temperature* b) -> std::optional<float> { return b->Bottom_temperature(); },
            70.0f,
            App_messages::Module_issue::IssueType::BottleBottomOverSensorTemp,
            "Bottle_bottom_sensor_temp_check",
            [bottle](const App_messages::Module_issue::Module_issue& issue) {
                auto copy = issue;  
                bottle->Send_CAN_message(copy);
            })
    {}
};