/**
 * @file can_thread.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 01.06.2024
 */

#pragma once


#include "thread.hpp"
#include "ticks.hpp"
#include "rtos/wrappers.hpp"

#include "can_bus/can_bus.hpp"
#include "can_bus/can_message.hpp"
#include "can_bus/app_message.hpp"

#ifndef CAN_DATA_TYPE
    #define CAN_DATA_TYPE etl::vector<uint8_t, 8>
#endif
#include "codes/messages/base_message.hpp"


#include "logger.hpp"

#include "etl/queue.h"

namespace fra = cpp_freertos;

/**
 * @brief   Thread responsible for management of CAN BUs peripheral
 *          Contains buffer for incoming and outgoing messages
 */
class CAN_thread : public fra::Thread {
public:
    /**
     * @brief Construct a new can thread object, also initialize CAN bus peripheral
     */
    explicit CAN_thread();

private:
    /**
     * @brief CAN bus peripheral based on can2040 library
     */
    CAN::Bus can_bus;

    /**
     * @brief   Size of queue for incoming and outgoing messages
     *          Messages which overflow this size are dropped
     */
    static const uint32_t queue_size = 64;

    /**
     * @brief  Queue for outgoing messages
     */
    etl::queue<CAN::Message, queue_size, etl::memory_model::MEMORY_MODEL_SMALL> tx_queue;

    /**
     * @brief  Queue for incoming messages
     */
    etl::queue<CAN::Message, queue_size, etl::memory_model::MEMORY_MODEL_SMALL> rx_queue;

protected:
    /**
     * @brief   Main function of thread, responsible for message handling, waits in loop for any IRQ from CAN bus peripheral
     */
    virtual void Run();

private:
    /**
     * @brief Executed when message if received stores it into rx queue
     *
     * @param message   Received message
     */
    void Receive(CAN::Message const &message);

    /**
     * @brief Executed when message is transmitted, if there is any message in tx queue, it is transmitted (put into peripheral buffer)
     *
     * @return uint8_t  Number of messages removed from tx queue
     */
    uint8_t Retransmit();

    /**
     * @brief   Menage error when happens, maybe
     *
     * @todo    Everything
     *
     * @param message   Message which is related to error
     * @return true     Error was handled
     * @return false    Error cannot be  handled
     */
    bool Error_management(CAN::Message const &message);

public:
    /**
     * @brief If peripheral is available for transmitting new message, it is sent, otherwise message is queued
     *
     * @param message   Message to be sent
     * @return uint     Number of messages in queue, if 0 then was send immediately
     */
    uint Send(CAN::Message const &message);

    /**
     * @brief If peripheral is available for transmitting new message, it is sent, otherwise message is queued
     *
     * @param message   Message to be sent
     * @return uint     Number of messages in queue, if 0 then was send immediately
     */
    uint Send(App_messages::Base_message &message);

    /**
     * @brief   Number of received messages in rx queue
     *
     * @return uint32_t Number of messages in rx queue
     */
    uint32_t Received_messages();

    /**
     * @brief   Check if there is any message in rx queue
     *
     * @return true     There is message in rx queue
     * @return false    There is no message in rx queue
     */
    bool Message_available() const;

    /**
     * @brief   Read message from rx queue, message is removed from queue
     *
     * @return std::optional<CAN::Message>   Received message if any was in queue, otherwise empty optional
     */
    std::optional<CAN::Message> Read_message();
};
