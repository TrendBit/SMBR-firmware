/**
 * @file heartbeat_thread.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 15.04.2024
 */

#pragma once

#include "thread.hpp"
#include "ticks.hpp"
#include "rtos/wrappers.hpp"

#include "hal/gpio/gpio.hpp"

namespace fra = cpp_freertos;

/**
 * @brief Green LED hearthbeat thread
 */
class LED_heartbeat_thread : public fra::Thread {
public:

    explicit LED_heartbeat_thread(uint gpio_led_number, uint32_t delay);

private:
    GPIO * const led;

    const uint32_t delay;

protected:
    virtual void Run();
};
