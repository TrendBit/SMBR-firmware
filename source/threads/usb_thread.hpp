/**
 * @file usb_thread.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 15.04.2024
 */

#pragma once

#include "thread.hpp"
#include "rtos/wrappers.hpp"
#include "tusb.h"
#include "usb_descriptors.h"

namespace fra = cpp_freertos;

/**
 * @brief   USB handler thread, in loop invokes TUSB device task
 *          Thread has lowest priority, runs instead of Idle task
 */
class USB_thread : public fra::Thread {
public:
    /**
     * @brief Construct a new usb thread
     */
    USB_thread();

protected:
    /**
     * @brief   Initialize TUSK device stack, and perform device task, Yield after every device task
     *              in order to give chance to other threads with higher priority to run
     */
    virtual void Run();

};
