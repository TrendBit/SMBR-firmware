/**
 * @file common_thread.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 14.08.2024
 */

#pragma once


#include "threads/can_thread.hpp"
#include "components/memory.hpp"
#include "logger.hpp"
#include "can_bus/message_router.hpp"

#include "thread.hpp"

namespace fra = cpp_freertos;

/**
 * @brief  This thread is present in all modules and is mainly responsible for handling of received messages via Message_router¨
 *             Does not spawn new thead for task processing, but perform them directly
 *             Task which are more complex or takes longer time should spawn new thread itself
 */
class Common_thread : public fra::Thread {
private:
    /**
     * @brief   Pointer to CAN bus manager thread which is responsible for handling of CAN Bus peripheral
     *          Messages are read from this thread and passed to router
     */
    CAN_thread * const can_thread;

    /**
     * @brief   Persistent memory containing calibration
     */
    EEPROM_storage * const memory;

public:
    /**
     * @brief Construct a new Common_thread object
     *
     * @param can_thread    Pointer to CAN bus manager thread which is responsible for handling of CAN Bus peripheral
     */
    Common_thread(CAN_thread * can_threadm, EEPROM_storage * const memory);

protected:
    /**
     * @brief   Main function of thread, executed after thread starts
     */
    virtual void Run();
};
