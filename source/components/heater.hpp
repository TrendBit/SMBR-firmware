/**
 * @file heater.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 28.11.2024
 */

#pragma once

#include "can_bus/message_receiver.hpp"
#include "components/component.hpp"
#include "components/motors/dc_hbridge_pio.hpp"
#include "components/thermometers/thermistor.hpp"
#include "hal/adc/adc_channel.hpp"
#include "hal/pio.hpp"
#include "logger.hpp"

#include "codes/codes.hpp"
#include "codes/messages/heater/set_intensity.hpp"
#include "codes/messages/heater/get_intensity_response.hpp"
#include "codes/messages/heater/set_target_temperature.hpp"
#include "codes/messages/heater/get_target_temperature_response.hpp"
#include "codes/messages/heater/get_plate_temperature_response.hpp"

class Heater: public Component, public Message_receiver {
private:
    /**
     * @brief H-bridge for control of heater
     */
    DC_HBridge_PIO * control_bridge;

    /**
     * @brief Maximal intensity of heater limited by heat dissipation of driver
     */
    const float intensity_limit = 0.8f;

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
