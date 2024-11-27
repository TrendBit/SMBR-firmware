/**
 * @file led_illumination.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 07.10.2024
 */

#pragma once

#include <vector>

#include "can_bus/app_message.hpp"
#include "can_bus/message_receiver.hpp"

#include "codes/codes.hpp"
#include "codes/messages/led_panel/set_intensity.hpp"
#include "codes/messages/led_panel/get_intensity_request.hpp"
#include "codes/messages/led_panel/get_intensity_response.hpp"
#include "codes/messages/led_panel/temperature_request.hpp"
#include "codes/messages/led_panel/temperature_response.hpp"

#include "components/component.hpp"
#include "components/led/led_intensity.hpp"
#include "components/common_sensors/thermistor.hpp"

#include "logger.hpp"

/**
 * @brief   LED_illumination is class for control of multiple LED channels with intensity control
 *          Each channel is represented by LED_intensity object, which can be controlled by this class
 *          Can be controlled by CAN messages received from CAN message router
 *          LED_illumination can be limited by power budget or temperature of module
 */
class LED_panel : public Component, Message_receiver{

    /**
     * @brief   Vector of pointers to LED_intensity objects, each representing one channel
     */
    std::vector<LED_intensity *> channels;

    /**
     * @brief   Pointer to temperature sensor object, which is used for temperature monitoring
     */
    Thermistor * const temp_sensor = nullptr;

    /**
     * @brief   Threshold temperature for temperature limitation of module
     *              above this temperature, LED intensity is scaled down
     */
    const float threshold_temperature = 80.0f;

    /**
     * @brief   Power budget of module, in watts
     */
    float power_budget_w = 0.0f;

public:
    /**
     * @brief   Construct a new led illumination object
     *
     * @param channels      Vector of pointers to LED_intensity objects, each representing one channel
     * @param temp_sensor   Pointer to temperature sensor object, which is used for temperature monitoring
     * @param power_budget  Power budget of module, in watts
     */
    LED_panel(std::vector<LED_intensity *> &channels, Thermistor * temp_sensor, float power_budget_w = 0.0f);

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

    /**
     * @brief   Set intensity of LED channel
     *
     * @param channel   Channel number of LED
     * @param intensity Intensity of LED, value from 0 to 1.0
     * @return true     Intensity was set
     * @return false    Intensity was not set, possibly channel out of range
     */
    bool Set_intensity(uint8_t channel, float intensity);

    /**
     * @brief   Get intensity of LED channel
     *
     * @param channel   Channel number of LED
     * @return float    Intensity of LED, value from 0 to 1.0
     */
    bool Get_intensity(uint8_t channel);

    /**
     * @brief  Response to request for temperature of LED panel
     *
     * @return true     Temperature was sent
     * @return false    Temperature was not sent, sensor not available
     */
    bool Get_temperature();

    /**
     * @brief   Detect if power of LED illumination is limited by power budget
     *
     * @return true     Power is limited
     * @return false    Power is not limited
     */
    bool Power_limited();

    /**
     * @brief   Detect if power of LED illumination is limited by temperature of module
     *
     * @return true     Temperature is limited
     * @return false    Temperature is not limited
     */
    bool Temperature_limited();

private:

    /**
     * @brief   Calculate power draw of all LED channels
     *
     * @return float    Power draw of all LED channels in watts
     */
    float Power_draw();

    /**
     * @brief   Calculate temperature of module
     *
     * @return float    Temperature of module in Celsius
     */
    float Temperature() const;
};
