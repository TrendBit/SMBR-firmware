/**
 * @file message_receiver.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 02.06.2024
 */

#pragma once

#include "can_message.hpp"

class CAN::Message_receiver {
public:
    Message_receiver() = default;

    virtual bool Receive(Message message) = 0;
};
