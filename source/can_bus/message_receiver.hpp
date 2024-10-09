/**
 * @file message_receiver.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 02.06.2024
 */

#pragma once

#include "can_bus/app_message.hpp"
#include "codes/codes.hpp"
#include "can_message.hpp"

#ifndef UNUSED
    #define UNUSED(x) (void)(x)
#endif

/**
 * @brief   Abstract class representing object which can receive CAN messages from router
 *          Register itself when created into Message router
 *          Derived class must implement receiving methods method
 */
class Message_receiver {
public:
    /**
     * @brief   Construct a new Message_receiver object
     *          Register itself into Message router to receive messages
     *
     * @param component  Type of component which is derived from this class, used for registration
     */
    explicit Message_receiver(Codes::Component component);

    /**
     * @brief   Method which is called when General/Admin message is received by router
     *
     * @param message   Message received by router determined for processing by this object
     * @return true     Message was processed by this object
     * @return false    Message cannot be processed by this object
     */
    virtual bool Receive(CAN::Message message) = 0;

    /**
     * @brief   Method which is called when Application message is received by router
     *
     * @param message   Message received by router determined for processing by this object
     * @return true     Message was processed by this object
     * @return false    Message cannot be processed by this object
     */
    virtual bool Receive(Application_message message) = 0;
};
