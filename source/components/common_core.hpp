/**
 * @file common_core.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 02.07.2024
 */

#pragma once

#include "can_bus/app_message.hpp"
#include "can_bus/message_receiver.hpp"
#include "threads/can_thread.hpp"
#include "hal/gpio/gpio.hpp"
#include "components/common_sensors/RP_internal_temperature.hpp"

#include "logger.hpp"

/**
 * @brief  This is component which is present in all modules and is responsible
 *              for processing of messages which must be supported by all modules
 *         Handles request like ping, mcu core temperature, load,  module discovery.
 */
class Common_core : public Message_receiver{
private:
    /**
     * @brief CAN periphery manager thread which is responsible for sending and receiving of CAN messages
     */
    CAN_thread * can_thread;

    /**
     * @brief Green on board LED used for signalization
     */
    GPIO * const green_led;

    /**
     * @brief Temperature sensor for measuring internal temperature of MCU
     */
    RP_internal_temperature * mcu_internal_temp;

public:
    /**
     * @brief Construct a new Common_core component, this includes registration to message router via Message_receiver interface
     *
     * @param can_thread    CAN periphery manager thread which is responsible for sending and receiving of CAN messages
     */
    explicit Common_core(CAN_thread * can_thread);

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
     * @brief   Process received ping request, send response with same sequence number
     *
     * @param message   Received ping request
     * @return true     Ping response was sent
     * @return false    Ping response cannot be sent (for example due to missing sequence number)
     */
    bool Ping(Application_message message);

    /**
     * @brief   Process received request for MCU core temperature, send response with current MCU core temperature
     *
     * @return true     Core temperature response was sent
     * @return false    Core temperature response cannot be sent
     */
    bool Core_temperature();
};
