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
#include "logger.hpp"

#include "etl/queue.h"

namespace fra = cpp_freertos;

/**
 * @brief Green LED hearthbeat thread
 */
class CAN_thread : public fra::Thread {
public:
    explicit CAN_thread();

private:
    CAN::Bus can_bus;

    static const uint32_t queue_size = 64;

    etl::queue<CAN::Message, queue_size, etl::memory_model::MEMORY_MODEL_SMALL> tx_queue;

    etl::queue<CAN::Message, queue_size, etl::memory_model::MEMORY_MODEL_SMALL> rx_queue;

protected:
    virtual void Run();

public:
    uint Send(CAN::Message const &message);

private:
    void Receive(CAN::Message const &message);

    uint8_t Retransmit();

    bool Error_management(CAN::Message const &message);
};
