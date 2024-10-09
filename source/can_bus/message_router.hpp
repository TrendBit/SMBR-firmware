/**
 * @file message_router.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 02.06.2024
 */

#pragma once

#include <unordered_map>

#include "codes/codes.hpp"
#include "logger.hpp"
#include "modules/base_module.hpp"
#include "can_bus/message_receiver.hpp"
#include "can_bus/routing_table.hpp"
#include "can_bus/app_message.hpp"
#include "can_bus/can_message.hpp"

#include "etl/unordered_map.h"

/**
 * @brief  Main hub for routing messages from CAN bus to correct receiver components of device
 *         Contains record book of registered components and their instances
 *         When CAN bus message if supplied to it it will determine if this module is received and pass
 *              it to corresponding component for processing.
 *         Routing rules (which component should receive which message) are defined in Routing_table
 */
class Message_router {
private:

    /**
     * @brief Map of device components and their instances
     *        Based on this map is determined on which received of component should be invoked
     */
    inline static etl::unordered_map<Codes::Component, Message_receiver *, 32> component_instances = {};

public:

    /**
     * @brief   If message is determined for this module then will route this message to correct component for processing
     *
     * @param message   Message for routing
     * @return true     Message was routed to correct component
     * @return false    Message cannot be routed to any component (this module is not receiver of this message, or component is not registered in router)
     */
    static bool Route(CAN::Message message);

    /**
     * @brief   Register instance of component into router as receiver for message types defined in Routing_table
     *
     * @param component    Component which will be registered
     * @param receiver     Instance of component (Message_receiver) which will receive messages
     */
    static void Register_receiver(Codes::Component component, Message_receiver *receiver);
};
