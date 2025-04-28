/**
 * @file bottle_temperature.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 21.01.2025
 */

#pragma once

#include "components/component.hpp"
#include "can_bus/message_receiver.hpp"
#include "components/adc/TLA2024.hpp"
#include "components/adc/TLA2024_channel.hpp"
#include "components/thermometers/thermopile.hpp"
#include "logger.hpp"

#include "codes/messages/bottle_temperature/temperature_response.hpp"
#include "codes/messages/bottle_temperature/top_measured_temperature_response.hpp"
#include "codes/messages/bottle_temperature/bottom_measured_temperature_response.hpp"
#include "codes/messages/bottle_temperature/top_sensor_temperature_response.hpp"
#include "codes/messages/bottle_temperature/bottom_sensor_temperature_response.hpp"


class Bottle_temperature: public Component, public Message_receiver {
private:
    Thermopile * const top_sensor;

    Thermopile * const bottom_sensor;

public:
    Bottle_temperature(Thermopile * top_sensor, Thermopile * bottom_sensor);

    bool temperature_initialized = false;

    float Temperature();

    float Top_temperature();

    float Bottom_temperature();

    float Bottom_sensor_temperature();

    float Top_sensor_temperature();

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
