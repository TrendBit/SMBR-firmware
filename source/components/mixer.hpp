/**
 * @file mixer.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 18.01.2025
 */

#pragma once

#include "can_bus/message_receiver.hpp"
#include "components/component.hpp"
#include "rtos/delayed_execution.hpp"
#include "logger.hpp"
#include "can_bus/app_message.hpp"
#include "rtos/repeated_execution.hpp"

#include "components/fan/fan_rpm.hpp"
#include "components/common_sensors/rpm_counter.hpp"
#include "components/common_sensors/rpm_counter_pio.hpp"

#include "codes/messages/mixer/set_speed.hpp"
#include "codes/messages/mixer/get_speed_request.hpp"
#include "codes/messages/mixer/get_speed_response.hpp"
#include "codes/messages/mixer/set_rpm.hpp"
#include "codes/messages/mixer/get_rpm_request.hpp"
#include "codes/messages/mixer/get_rpm_response.hpp"
#include "codes/messages/mixer/stir.hpp"
#include "codes/messages/mixer/stop.hpp"

/**
 * @brief   Mixer component is realized using fan with pwm controlled speed and techometric sensor for measurement of speed
 */
class Mixer: public Component, public Message_receiver, private Fan_RPM {
private:
    /**
     * @brief   Maximum RPM of mixer at full speed
     */
    float max_rpm;

    /**
     * @brief   Minimum speed at which is pump moving of pump in range 0-1
     *          Opposite direction is assumed to be the same just negative
     */
    float min_speed = 0.0f;

    /**
     * @brief  Proportional gain of regulation loop
     */
    float p_gain = 0.05f;

    float target_rpm = 0.0f;

    /**
     * @brief Timer function which stops mixer after defined time, used for stirring command
     */
    rtos::Delayed_execution * mixer_stopper;

    /**
     *  @brief Loop regulating intensity of heater based on temperature of bottle
     */
    rtos::Repeated_execution *regulation_loop;

public:
    /**
     * @brief Construct a new Mixer object
     *
     * @param pwm_pin       GPIO pin for control of mixer
     * @param tacho         RPM counter for measurement of mixer speed
     * @param frequency     Frequency of PWM signal for control of mixer
     * @param max_rpm       Maximum RPM of mixer at full speed
     * @param min_speed     Minimum speed at which is fan moving in range 0-1
     */
    Mixer(uint8_t pwm_pin, RPM_counter* tacho, float frequency, float max_rpm, float min_speed = 0.0f);

    /**
     * @brief   Set speed of mixer in range from 0 to 1.0
     *
     * @param speed     Speed of mixer in range from 0 to 1.0
     * @return float    Target speed of mixer in range from 0 to 1.0
     */
    float Speed(float speed);

    /**
     * @brief   Get current speed of mixer in range from 0 to 1.0
     *
     * @return float    Speed of mixer in range from 0 to 1.0
     */
    float Speed();

    /**
     * @brief           Set speed of mixer in RPM, target RPM can be limited by min or max speed
     *
     * @param rpm       Speed of mixer in RPM
     * @return float    Target speed of mixer in RPM
     */
    float RPM(float rpm);

    /**
     * @brief   Get current speed of mixer in RPM, derived from RPM counter
     */
    using Fan_RPM::RPM;

    /**
     * @brief   Stop mixer
     */
    void Stop();

    /**
     * @brief   Stir liquid in bottle with defined RPM for given time
     *
     * @param rpm       RPM of mixer
     * @param time      Time of stirring in seconds
     */
    void Stir(float rpm, float time_s);

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

private:

    void Regulation_loop();

};
