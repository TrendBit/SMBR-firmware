/**
 * @file component.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 26.11.2024
 */

#pragma once

#include "codes/codes.hpp"

#include "codes/messages/base_message.hpp"
#include "can_bus/can_message.hpp"

#include "etl/vector.h"

typedef unsigned int uint;

/**
 * @brief Class representing abstract component, which is part of module
 */
class Component {
private:
    /**
     * @brief Type of component
     */
    Codes::Component component;

    /**
     * @brief List of available components
     */
    inline static etl::vector<Codes::Component, 256> available_components;

public:
    /**
     * @brief Construct a new Component object
     *
     * @param component Type of component
     */
    explicit Component(Codes::Component component);

    /**
     * @brief   Return type of component
     *
     * @return Codes::Component Type of component
     */
    Codes::Component Component_type() const;

    /**
     * @brief   Send application message via can bus
     *          This is wrapper around Base_m,odule wrapper for can_thread
     *
     * @param message   Message to send
     * @return uint     Number of messages waiting in a buffer to be send
     */
    uint Send_CAN_message(App_messages::Base_message &message);

    uint Send_CAN_message(CAN::Message &message);

    /**
     * @brief   Return list of available components
     *
     * @return etl::vector<Codes::Component, 256> List of available components
     */
    inline static etl::vector<Codes::Component, 256> Available_components();
};
