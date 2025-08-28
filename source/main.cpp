#include "main.hpp"

namespace fra = cpp_freertos;

int main(){
    timer_hw->dbgpause = 0; // Required for SWD debug otherwise timers are alway zero during debug

    #ifdef CONFIG_WATCHDOG
        watchdog_enable(5000, 1);
    #endif

    #ifdef CONFIG_LOGGER
        Logger(static_cast<Logger::Level>(CONFIG_LOGGER_LEVEL), Logger::Color_mode::Prefix);
        Logger::Init_UART(uart0, 0, 1, 961200);
        Logger::Init_USB(1);
        Logger::Print_raw("\r\n");
        Logger::Critical("Device start");
        Logger::Notice("Logger UART Initialized");
        if(watchdog_enable_caused_reboot()){
            Logger::Error("Watchdog caused reboot");
        }
    #endif

    new USB_thread();
    new CLI_service();

    #ifdef CONFIG_CONTROL_MODULE
        new Control_module();
    #elifdef CONFIG_SENSOR_MODULE
        new Sensor_module();
    #elifdef CONFIG_PUMP_MODULE
        new Pump_module();
    #else
        #error "No module defined, use 'make menuconfig' to select module"
    #endif

    fra::Thread::StartScheduler();
}
