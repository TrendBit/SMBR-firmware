/**
 * @file fans.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 09.10.2024
 */

#pragma once

#include "etl/vector.h"

#include <optional>

#include "can_bus/message_receiver.hpp"

class Fan : public Message_receiver {
private:
    etl::vector<float, 8> channels;
public:
    Fan(etl::vector<float, 8> &channels);

    bool Receive(CAN::Message message) override final;

    bool Receive(Application_message message) override final;

    bool Set_intensity(uint8_t channel, float intensity);

    bool Set_rpm(uint8_t channel, float rpm);

    std::optional<float> Get_rpm(uint8_t channel);

};
