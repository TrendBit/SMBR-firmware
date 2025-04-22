/**
 * @file fluorometer_thread.hpp
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
#include "components/fluorometer.hpp"
#include "codes/messages/fluorometer/ojip_capture_request.hpp"
#include "codes/messages/fluorometer/calibration_request.hpp"

namespace fra = cpp_freertos;

class Fluorometer_thread : public fra::Thread {
private:
    /**
     * @brief   Pointer to fluorometer object
     */
    Fluorometer * const fluorometer;

    /**
     * @brief   Buffer for storing messages received from CAN bus before processing then in FIFO order
     */
    etl::queue<Application_message, 32, etl::memory_model::MEMORY_MODEL_SMALL> message_buffer;

    /**
     * @brief   List of messages supported for processing by this thread
     */
    const etl::array<Codes::Message_type, 3> supported_messages = {
        Codes::Message_type::Fluorometer_OJIP_capture_request,
        Codes::Message_type::Fluorometer_OJIP_retrieve_request,
        Codes::Message_type::Fluorometer_calibration_request,
    };

public:
    explicit Fluorometer_thread(Fluorometer * const fluorometer);

    bool Enqueue_message(Application_message &message);

protected:
    /**
     * @brief   Main function of thread, executed after thread starts
     */
    virtual void Run();
};
