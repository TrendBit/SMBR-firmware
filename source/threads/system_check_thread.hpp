#pragma once

#include "thread.hpp"
#include "logger.hpp"

class Led_temperature_check;  
class LED_panel;
class Board_temperature_check;
class Base_module;

namespace fra = cpp_freertos;

/**
 * @brief System check thread for monitoring system
 */
class System_check_thread : public fra::Thread {
public:
    /**
     * @brief Constructs the system check thread
     * @param panel Pointer to the LED panel to be monitored
     */
    System_check_thread(Base_module* module, LED_panel* panel);

    void AttachLedPanel(LED_panel* panel);

protected:
    /**
     * @brief Main thread execution function
     */
    virtual void Run() override;

private:
    /**
     * @brief Pointer to the temperature check component
     */
    Led_temperature_check* led_check;

    Board_temperature_check* board_check;
};
