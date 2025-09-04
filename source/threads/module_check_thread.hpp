#pragma once

#include "module_check/IModuleCheck.hpp"
#include "thread.hpp"
#include "logger.hpp"

class Led_temperature_check;  
class LED_panel;
class Board_temperature_check;
class Base_module;

namespace fra = cpp_freertos;

/**
 * @brief Module check thread for monitoring system
 */
class Module_check_thread : public fra::Thread {
public:
    /**
     * @brief Constructs the module check thread
     */
    explicit Module_check_thread();

    void AttachCheck(IModuleCheck* check);

protected:
    /**
     * @brief Main thread execution function
     */
    void Run() override;

private:
    std::vector<IModuleCheck*> checks;
};
