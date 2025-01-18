/**
 * @file cuvette_pump.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 19.11.2024
 */

#pragma once

#include <algorithm>

#include "can_bus/message_receiver.hpp"
#include "components/component.hpp"
#include "components/motors/peristaltic_pump.hpp"
#include "rtos/delayed_execution.hpp"
#include "logger.hpp"
#include "can_bus/app_message.hpp"


#include "codes/messages/cuvette_pump/set_speed.hpp"
#include "codes/messages/cuvette_pump/get_speed_request.hpp"
#include "codes/messages/cuvette_pump/get_speed_response.hpp"
#include "codes/messages/cuvette_pump/set_flowrate.hpp"
#include "codes/messages/cuvette_pump/get_flowrate_request.hpp"
#include "codes/messages/cuvette_pump/get_flowrate_response.hpp"
#include "codes/messages/cuvette_pump/move.hpp"
#include "codes/messages/cuvette_pump/prime.hpp"
#include "codes/messages/cuvette_pump/purge.hpp"
#include "codes/messages/cuvette_pump/stop.hpp"

/**
 * @brief   Wrapper for peristaltic pump which controls suspense in cuvette
 *          In addition to the normal peristaltic pump allows to prime and purge cuvette system
 */
class Cuvette_pump: public Component, public Message_receiver, private Peristaltic_pump{
private:
    /**
     * @brief  Volume of cuvette system in ml, this amount will be primed or purged
     */
    const float cuvette_system_volume;

    /**
     * @brief Timer function which stops pump after defined time, used for moving volumes of liquid
     */
    rtos::Delayed_execution * pump_stopper;

public:
    /**
     * @brief Construct a new Cuvette_pump object
     *
     * @param gpio_in1      GPIO pin for control of first input of H-bridge, Forward
     * @param gpio_in2      GPIO pin for control of second input of H-bridge, Reverse
     * @param max_flowrate              Maximum flowrate of pump in ml/min
     * @param cuvette_system_volume     Volume of cuvette system in ml
     * @param min_speed     Minimum speed at which is pump moving of pump in range 0-1
     * @param pwm_frequency             Frequency of PWM signal for control of motor
     */
    Cuvette_pump(uint gpio_in1, uint gpio_in2, float max_flowrate, float cuvette_system_volume, float min_speed = 0, float pwm_frequency = 50.0f);

    /**
     * @brief Set speed of pump, derived from Peristaltic_pump::Speed
     */
    using Peristaltic_pump::Speed;

    /**
     * @brief Set flowrate of pump, derived from Peristaltic_pump::Flowrate
     */
    using Peristaltic_pump::Flowrate;

    /**
     * @brief   Stop pump immediately by coasting
     */
    void Stop();

    /**
     * @brief   Move given volume of liquid with given flowrate into the cuvette or out of cuvette when volume is negative
     *          Maximal flowrate of pump is used
     *
     * @param volume_ml Volume of liquid to move in ml (negative value means move out of cuvette)
     * @return float    Time of pumping in seconds
     */
    float Move(float volume_ml);

    /**
     * @brief   Move given volume of liquid with given flowrate into the cuvette or out of cuvette when volume is negative
     *          Flowrate can be configured by parameter
     *
     * @param volume_ml Volume of liquid to move in ml (negative value means move out of cuvette)
     * @param flowrate  Flowrate of pump in ml/min
     * @return float    Time of pumping in seconds
     */
    float Move(float volume_ml, float flowrate);

    /**
     * @brief   Prime cuvette system, move liquid from bottle to cuvette
     *
     * @return float    Time of pumping in seconds
     */
    float Prime();

    /**
     * @brief   Purge cuvette system, move liquid from cuvette to bottle
     *
     * @return float    Time of pumping in seconds
     */
    float Purge();

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
