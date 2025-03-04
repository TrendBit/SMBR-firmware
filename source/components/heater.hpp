/**
 * @file heater.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 28.11.2024
 */

#pragma once

#include <algorithm>

#include "can_bus/message_receiver.hpp"
#include "can_bus/message_router.hpp"
#include "components/component.hpp"
#include "components/motors/dc_hbridge_pio.hpp"
#include "components/thermometers/thermistor.hpp"
#include "hal/adc/adc_channel.hpp"
#include "hal/pio.hpp"
#include "logger.hpp"
#include "rtos/repeated_execution.hpp"
#include "rtos/delayed_execution.hpp"

#include "codes/codes.hpp"
#include "codes/messages/bottle_temperature/temperature_request.hpp"
#include "codes/messages/bottle_temperature/temperature_response.hpp"
#include "codes/messages/heater/get_intensity_response.hpp"
#include "codes/messages/heater/get_plate_temperature_response.hpp"
#include "codes/messages/heater/get_target_temperature_response.hpp"
#include "codes/messages/heater/set_intensity.hpp"
#include "codes/messages/heater/set_target_temperature.hpp"

class Heater: public Component, public Message_receiver {
private:
    /**
     * @brief   Point in compensation curve of heater intensity
     */
    struct PowerPoint {
        float set;
        float out;
    };

    /**
     * @brief Power curve of heater to compensate nonlinearity of heater intensity
     */
    static constexpr std::array<PowerPoint, 11> power_curve = {{
        {0.0, 0.00},
        {0.1, 0.05},
        {0.2, 0.07},
        {0.3, 0.08},
        {0.4, 0.09},
        {0.5, 0.11},
        {0.6, 0.16},
        {0.7, 0.28},
        {0.8, 0.46},
        {0.9, 0.67},
        {1, 1.00}
    }};

    /**
     * @brief H-bridge for control of heater
     */
    DC_HBridge_PIO * control_bridge;

    /**
     * @brief Maximal intensity of heater limited by heat dissipation of driver
     *        Higher intensity will not help to reach lower temperatures due to heat creapage though peltier
     */
    const float intensity_limit = 0.7f;

    /**
     * @brief  Proportional gain of regulation loop
     */
    float p_gain = 0.25f;

        /**
     * @brief Integral gain of regulation loop
     */
    float i_gain = 0.01f;

    /**
     * @brief Accumulated integral error for I component of PI controller
     */
    float integral_error = 0.0f;

    /**
     * @brief Anti-windup limit to prevent integral term from growing too large
     */
    float integral_limit = 10.0f;

    /**
     * @brief Maximal step size which can be done in one iteration of regulation loop
     */
    float regulation_step = 0.05f;

    /**
     * @brief Current intensity of heater
     */
    float intensity = 0.0;


    /**
     * @brief   Thermistor for temperature measurement of heatspreader of heater
     */
    Thermistor * const heater_sensor;

    /**
     * @brief GPIO for control of heater fan (fan regulation temperature of heater heatsink)
     */
    GPIO * const heater_fan;

    /**
     * @brief   Target temperature which should be reached via regulator
     *          If not set, regulation is disabled
     */
    std::optional<float> target_temperature = std::nullopt;

    /**
     *  @brief Loop regulating intensity of heater based on temperature of bottle
     */
    rtos::Repeated_execution *regulation_loop;

    /**
     * @brief   temperature of bottle obtained from can bus message
     */
    std::optional<float> bottle_temperature = std::nullopt;

public:
    /**
     * @brief Construct a new Heater object
     *
     * @param gpio_in1      GPIO number of input 1 of H-bridge, Forward
     * @param gpio_in2      GPIO number of input 2 of H-bridge, Reverse
     * @param pwm_frequency Frequency of PWM signal for control of heater, around 100K Hz seems optimal
     */
    Heater(uint gpio_in1, uint gpio_in2, float pwm_frequency);

    /**
     * @brief   Set intensity of heater, positive value means heating, negative cooling
     *
     * @param requested_intensity   Intensity of heater, range <-1, 1>
     * @return float            Real intensity set to heater, limited by maximal intensity (power dissipation of driver)
     */
    float Intensity(float requested_intensity);

    /**
     * @brief   Get current intensity of heater
     *
     * @return float    Current intensity of heater
     */
    float Intensity() const;

    /**
     * @brief Read temperature from thermistor connected to heatspreader of heater
     *
     * @return float    Temperature in Celsius
     */
    float Temperature();

    /**
     * @brief   Immediately turn off heater, set intensity to 0 and disable regulation (clear target temperature)
     */
    void Turn_off();

private:
    /**
     * @brief   Linearize power curve of heater to compensate nonlinearity of heater intensity
     *
     * @param requested_intensity   Requested intensity of heater
     * @return float                Corresponding intensity of heater
     */
    float Compensate_intensity(float requested_intensity);

    /**
     * @brief  Sends request for bottle temperature used to regulate heater
     *
     * @return true     Request was sent
     * @return false    Request was not sent
     */
    bool Request_bottle_temperature();

    /**
     * @brief   Main loop of regulation of heater intensity based on bottle temperature
     *          Executed from Repeated_execution lambda
     */
    void Regulation_loop();

protected:

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
