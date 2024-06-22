/**
 * @file message_router.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 02.06.2024
 */

#pragma once

#include "codes.hpp"
#include "can_bus/message_receiver.hpp"

#include "etl/unordered_map.h"

class Message_router {
private:
    static etl::unordered_map<Command, Component> Command_receivers = {
        { Command::Serial_probe,             Component::CAN_serial },
        { Command::Serial_ID_respond,        Component::CAN_serial },
        { Command::Serial_port_confirmation, Component::CAN_serial },
        { Command::Discover_probe,           Component::Core },
        { Command::Discover_respond,         Component::Core },
    };

    static etl::unordered_map<Component, CAN::Message_receiver *> Component_instances = {
        { Component::CAN_serial, null_ptr},
    };

public:
    Message_router() = default;

    bool Route(CAN::Message message){
        auto receiver = Command_receivers.find(message.id);
        if (receiver != Command_receivers.end()){
            return Component_instances[receiver->second]->Receive(message);
        }
        return false;
    }

    void Register_receiver(Component component, *CAN::Message_receiver receiver){
        CAN::Message_receiver = Component_instances[component];
        if (receiver != nullptr){
            Component_instances[component] = receiver;
        }
    }
};
