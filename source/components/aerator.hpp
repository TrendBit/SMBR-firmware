/**
 * @file aerator.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 18.01.2025
 */

#pragma once

#include <algorithm>

#include "can_bus/message_receiver.hpp"
#include "components/component.hpp"
#include "components/motors/peristaltic_pump.hpp"
#include "rtos/delayed_execution.hpp"
#include "logger.hpp"
#include "can_bus/app_message.hpp"

#include "codes/messages/aerator/set_speed.hpp"
#include "codes/messages/aerator/set_flowrate.hpp"
#include "codes/messages/aerator/get_speed_request.hpp"
#include "codes/messages/aerator/get_speed_response.hpp"
#include "codes/messages/aerator/get_flowrate_request.hpp"
#include "codes/messages/aerator/get_flowrate_response.hpp"
#include "codes/messages/aerator/move.hpp"
#include "codes/messages/aerator/stop.hpp"

/**
 * @brief   Aerator is component which is used to aerate liquid in bottle
 *          Membrane air pump is used with DC motor controlled by H-bridge
 *          Movement of motor is possible only in one direction (pumping air into bottle)
 *              Otherwise liquid would be sucked into pump membrane
 */
class Aerator: public Component, public Message_receiver, private DC_HBridge {
private:
    /**
     * @brief   Maximum flowrate of air pump in ml/min
     */
    float max_flowrate;

    /**
     * @brief   Minimum speed at which is pump moving of pump in range 0-1
     *          Opposite direction is assumed to be the same just negative
     */
    float min_speed = 0.0f;

    /**
     * @brief Timer function which stops pump after defined time, used for moving volumes of liquid
     */
    rtos::Delayed_execution * pump_stopper;

public:
    /**
     * @brief Construct a new Aerator object
     *
     * @param gpio_in1      GPIO pin for control of first input of H-bridge, Forward
     * @param gpio_in2      GPIO pin for control of second input of H-bridge, Reverse
     * @param max_flowrate  Maximum flowrate of pump in ml/min
     * @param min_speed     Minimum speed at which is pump moving of pump in range 0-1
     * @param pwm_frequency Frequency of PWM signal for control of motor
     */
    Aerator(uint gpio_in1, uint gpio_in2, float max_flowrate, float min_speed = 0.0f, float pwm_frequency = 50.0f);

    /**
     * @brief Set and get speed of pump, derived from DC_HBridge::Speed
     */
    using DC_HBridge::Speed;

    /**
     * @brief   Set flowrate of pump
     *
     * @param flowrate  Flowrate of pump in ml/min, limited to max flow rate of pump
     * @return float    Flowrate of pump in ml/min
     */
    float Flowrate(float flowrate);

    /**
     * @brief   Get current flowrate of pump
     *
     * @return float    Current flowrate of pump in ml/min
     */
    float Flowrate();

    /**
     * @brief   Move given volume of air with maximal flowrate
     *          Only positive values of volume are allowed
     *
     * @param volume_ml Volume of air to move in ml
     * @return float    Time of pumping in seconds
     */
    float Move(float volume_ml);

    /**
     * @brief   Move given volume of air with given flowrate
     *          Only positive values of volume and flowrate are allowed
     *
     * @param volume_ml     Volume of air to move in ml
     * @param flowrate      Flowrate of pump in ml/min
     * @return float        Time of pumping in seconds
     */
    float Move(float volume_ml, float flowrate);

    /**
     * @brief   Stop pump immediately by coasting
     */
    void Stop();

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
