/**
 * @file pumps.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 12.12.2025
 */

#pragma once

#include "can_bus/message_receiver.hpp"
#include "component.hpp"
#include "logger.hpp"
#include "hal/gpio/gpio.hpp"
#include "hal/pwm/pwm.hpp"
#include "components/motors/dc_hbridge.hpp"
#include "rtos/execute_until.hpp"

#include "codes/messages/pumps/pump_count_response.hpp"
#include "codes/messages/pumps/set_speed.hpp"

class Pump: private DC_HBridge{
private:
    /**
     * @brief   PWM channel used to indicate pump activity
     */
    PWM_channel * indication;

public:
    /**
     * @brief Construct a new Pump object
     *
     * @param gpio_in1          GPIO pin for control of first input of H-bridge, Forward
     * @param gpio_in2          GPIO pin for control of second input of H-bridge, Reverse
     * @param indication_pin        GPIO pin visualization of pump activity
     * @param max_flowrate      Maximum flowrate of pump in ml/min
     * @param min_speed         Minimum speed at which is pump moving of pump in range 0-1
     * @param pwm_frequency     Frequency of PWM signal for control of motor
     */
    Pump(uint8_t gpio_in1, uint8_t gpio_in2, uint8_t indication_pin, float max_flowrate, float min_speed = 0, float pwm_frequency = 50.0f);

    /**
     * @brief   Indicate pump activity by setting intensity of indication LED
     *
     * @param intensity     Intensity of indication LED from 0.0 to 1.0
     */
    void Indicate(float intensity);

    /**
     * @brief   Set speed of pump, overrides DC_HBridge Speed method to add indication
     *
     * @param speed     Speed of pump from -1.0 to 1.0
     */
    virtual void Speed(float speed) override final;
};

class Pump_controller: public Component, public Message_receiver {
private:
    /**
     * @brief   List of pumps connected to module
     */
    etl::vector<Pump *,8> pumps;

public:
    /**
     * @brief Construct a new Pump_controller component
     */
    Pump_controller(etl::vector<Pump *,8> pumps);

    /**
     * @brief   Number of pump connected to module, detection is based on configuration pin level
     */
    uint8_t Pump_count(){return pumps.size();};

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
