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
 * @brief Yellow LED hearth-beat thread signaling that device is alive
 */
class Heartbeat_thread : public fra::Thread {
public:
    /**
     * @brief Construct a new led heartbeat thread and starts is
     *
     * @param gpio_led_number   Number of GPIO pin where LED is connected
     * @param delay             Delay between LED state change
     */
    explicit Heartbeat_thread(uint gpio_led_number, uint32_t delay);

private:
    /**
     * @brief Pointer to GPIO object representing LED
     */
    GPIO * const led;

    /**
     * @brief Delay between LED state change
     */
    const uint32_t delay;

protected:
    /**
     * @brief   Main function of thread, executed after thread starts
     */
    virtual void Run();
};
