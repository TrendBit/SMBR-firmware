/**
 * @file spectrophotometer_thread.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 18.04.2025
 */

#pragma once

#include "thread.hpp"
#include "codes/codes.hpp"
#include "logger.hpp"
#include "etl/queue.h"
#include "etl/array.h"
#include "can_bus/app_message.hpp"
#include "components/spectrophotometer.hpp"
#include "codes/messages/spectrophotometer/measurement_request.hpp"

namespace fra = cpp_freertos;

class Spectrophotometer_thread : public fra::Thread {
private:
    /**
     * @brief   Pointer to spectrophotometer object
     */
    Spectrophotometer * const spectrophotometer;

    /**
     * @brief   Buffer for storing messages received from CAN bus before processing then in FIFO order by thread
     */
    etl::queue<Application_message, 32, etl::memory_model::MEMORY_MODEL_SMALL> message_buffer;

    /**
     * @brief   List of messages supported for processing by this thread
     */
    const etl::array<Codes::Message_type, 2> supported_messages = {
        Codes::Message_type::Spectrophotometer_measurement_request,
        Codes::Message_type::Spectrophotometer_calibrate
    };

public:

    explicit Spectrophotometer_thread(Spectrophotometer * const spectrophotometer);

    bool Enqueue_message(Application_message &message);

protected:
    /**
     * @brief   Main function of thread, executed after thread starts
     */
    virtual void Run();
};
