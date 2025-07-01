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

#include "qlibs.h"
#include "etl/queue.h"

class Mixer_rpm_filter;

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
     * @brief   PID controller for regulation of mixer speed
     */
    qlibs::pidController * control;

    /**
     * @brief   First stage filter for RPM
     *          Reduces effect of incorrect RPM reading at low speeds due to control signal alias
     */
    qlibs::smootherLPF2 * filter1;

    /**
     * @brief   Second stage filter for RPM
     *          General smoothing filter for RPM measurement
     */
    qlibs::smootherLPF2 * filter2;

    Mixer_rpm_filter * rpm_filter;

    /**
     * @brief   Window for moving average of RPM, used for smoothing of RPM measurement
     *          This is used to reduce noise in RPM measurement
     */
    float window[10] = {0.0f};

    /**
     * @brief   Minimum speed at which is mixer moving in range 0-1
     *          This assumes magnetic stirrer is placed inside algae culture in bottle.
     */
    float min_speed = 0.3f;

    /**
     * @brief   Target RPM of mixer
     */
    float target_rpm = 0.0f;

    /**
     * @brief
     *
     */
    float current_rpm = 0.0f;

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
    virtual float RPM() override;

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

class Mixer_rpm_filter{

    qlibs::smootherLPF2 input_filter;

    qlibs::smootherLPF2 output_filter;

    const float accept_threshold = 0.95f;

    float filter_value = 0.0f;

public:
    Mixer_rpm_filter(){
        input_filter.setup(0.3f);
        output_filter.setup(0.1f);
    }

    float Value() const {
        return filter_value;
    }

    float Smooth(float input_value){
        float rpm = 0;
        if(roundf(input_value) != 300){
            rpm = input_value;
        }

        float smooth_input = input_filter.smooth(rpm);

        if((input_value > (accept_threshold * smooth_input)) or (smooth_input < 5.0f)){
            filter_value = output_filter.smooth(rpm);
        }

        // Logger::Notice("Input={:4.1f}, InputF={:4.1f}, OutputF={:4.1f}", roundf(input_value), smooth_input, filter_value);

        return filter_value;
    }

};
