#pragma once

#include "system_check/ISystemCheck.hpp"
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
     */
    explicit System_check_thread();

    void AttachCheck(ISystemCheck* check);

protected:
    /**
     * @brief Main thread execution function
     */
    void Run() override;

private:
    std::vector<ISystemCheck*> checks;
};
