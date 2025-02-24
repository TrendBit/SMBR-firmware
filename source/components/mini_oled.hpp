/**
 * @file mini_oled.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 20.01.2025
 */

#pragma once

#include "can_bus/message_receiver.hpp"
#include "can_bus/message_router.hpp"
#include "components/component.hpp"
#include "components/bottle_temperature.hpp"
#include "rtos/repeated_execution.hpp"
#include "rtos/delayed_execution.hpp"
#include "logger.hpp"
#include "threads/mini_display_thread.hpp"

#include "codes/messages/core/sid_response.hpp"
#include "codes/messages/core/serial_response.hpp"
#include "codes/messages/core/hostname_response.hpp"
#include "codes/messages/core/ip_response.hpp"
#include "codes/messages/mini_oled/clear_custom_text.hpp"
#include "codes/messages/mini_oled/print_custom_text.hpp"
#include "codes/messages/bottle_temperature/temperature_request.hpp"
#include "codes/messages/bottle_temperature/temperature_response.hpp"
#include "codes/messages/heater/get_plate_temperature_request.hpp"
#include "codes/messages/heater/get_plate_temperature_response.hpp"
#include "codes/messages/heater/get_target_temperature_request.hpp"
#include "codes/messages/heater/get_target_temperature_response.hpp"

/**
 * @brief   Component requesting and reading device info from can bus and displaying them on OLED display
 */
class Mini_OLED: public Component, public Message_receiver {
private:
    /**
     * @brief Rate of data update in seconds
     */
    uint32_t data_update_rate_s;
public:
    /**
     * @brief Construct a new Mini_OLED object
     *
     * @param data_update_rate_s    Rate of data update in seconds
     */
    Mini_OLED(Bottle_temperature * const bottle_temp_sensor, uint32_t data_update_rate_s = 30);

    /**
     * @brief Thread responsible for display rendering
     */
    Mini_display_thread * const lvgl_thread;

    /**
     * @brief  Repeatedly executed function which requests data from core module
     */
    rtos::Repeated_execution *update_data;

    /**
     * @brief   Pointer to temperature sensor of bottle which supplies temperature data for display
     */
    Bottle_temperature * const bottle_temp_sensor;

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
