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
#include "codes/messages/mixer/info_response.hpp"

#include "qlibs.h"
#include "etl/queue.h"

class Mixer_rpm_filter;

/**
 * @brief   Mixer component is realized using fan with pwm controlled speed and techometric sensor for measurement of speed
 */
class Mixer: public Component, public Message_receiver, private Fan_RPM {
private:
    /**
     * @brief   Minimal RPM of mixer which can be reliable regulated with load
     */
    const float min_rpm;

    /**
     * @brief   Maximum RPM of mixer at full speed
     *          This value is limited by used magnetic stirred and density of liquid.
     *          Generaly wil be much lower than real maximum RPM of fan without load.
     */
    const float max_rpm;

    /**
     * @brief   PID controller used for regulation of mixer speed
     */
    qlibs::pidController * control;

    /**
     * @brief   Two stage RPM filter used for control of mixer rpm
     */
    Mixer_rpm_filter * rpm_filter;

    /**
     * @brief   Target RPM of mixer
     */
    float target_rpm = 0.0f;

    /**
     * @brief   Current filtered RPM of mixer
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
     * @param min_rpm       Minimum speed at which mixer can be reliably regulated
     * @param max_rpm       Maximum RPM of fan at full speed (without load)
     */
    Mixer(uint8_t pwm_pin, RPM_counter* tacho, float frequency, float min_rpm = 300.0f, float max_rpm = 6000.0f);

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
    /**
     * @brief   Periodically executed function which filters RPMs and regulates speed of mixer is target RPM is set
     */
    void Regulation_loop();

    /**
     * @brief   Maximal supported speed of mixer in RPM
     *          Valid when magnetic stirrer is placed inside algae culture in bottle in reactor.
     *          This is used to limit speed of mixer when setting speed in range 0-
     *
     * @return float
     */
    float Max_speed() const { return max_rpm;}

    /**
     * @brief   Minimal supported speed of mixer in RPM
     *          Valid when magnetic stirrer is placed inside algae culture in bottle in reactor.
     *          Lower values then this cannot be reliably measured and regulated.
     *
     * @return float
     */
    float Min_speed() const { return min_rpm;}

};

/**
 * @brief   Dual stage low-pass filter for RPM measurement
 *          First stage filter is used to reduce effect of incorrect RPM reading at low speeds.
 *          Second stage filter is used for general smoothing of RPM measurement for control purposes.
 */
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
