/**
 * @file mini_oled.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 20.01.2025
 */

#pragma once

#include "can_bus/message_receiver.hpp"
#include "components/component.hpp"
#include "rtos/delayed_execution.hpp"
#include "logger.hpp"
#include "threads/mini_display_thread.hpp"

#include "codes/messages/core/sid_response.hpp"
#include "codes/messages/core/serial_response.hpp"
#include "codes/messages/core/hostname_response.hpp"
#include "codes/messages/core/ip_response.hpp"

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
    Mini_OLED(uint32_t data_update_rate_s = 30);

    /**
     * @brief Thread responsible for display rendering
     */
    Mini_display_thread * const lvgl_thread;

    /**
     * @brief  Repeatedly executed function which requests data from core module
     */
    rtos::Delayed_execution *update_data;

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
