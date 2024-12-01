/**
 * @file heater.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 28.11.2024
 */

#pragma once

#include "codes/codes.hpp"

#include "can_bus/message_receiver.hpp"

#include "components/component.hpp"
#include "components/motors/dc_hbridge.hpp"

class Heater: public Component, public Message_receiver {
private:
    DC_HBridge * control_bridge;

    float intensity = 0.0;

public:
    Heater(uint gpio_in1, uint gpio_in2, float pwm_frequency = 50.0f);

    float Intensity(float requested_intensity);

    /**
     * @brief   Receive message implementation from Message_receiver interface for General/Admin messages (normal frame)
     *          This method is invoked by Message_router when message is determined for this component
     *
     * @todo    Implement all admin messages (enumeration, reset, bootloader)
     *
     * @param message   Received message
     * @return true     Message was processed by this component
     * @return false    Message cannot be processed by this component
     */
    virtual bool Receive(CAN::Message message) override final;

    /**
     * @brief   Receive message implementation from Message_receiver interface for Application messages (extended frame)
     *          This method is invoked by Message_router when message is determined for this component
     *
     * @param message   Received message
     * @return true     Message was processed by this component
     * @return false    Message cannot be processed by this component
     */
    virtual bool Receive(Application_message message) override final;

};
