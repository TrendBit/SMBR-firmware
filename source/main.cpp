#include "main.hpp"
#include "rtos/wrappers.hpp"
#include "info.hpp"

namespace fra = cpp_freertos;

int main(){
    timer_hw->dbgpause = 0; // Required for SWD debug otherwise timers are alway zero during debug

    #ifdef CONFIG_WATCHDOG
        watchdog_enable(5000, 1);
    #endif

    #if defined(CONFIG_LOGGER_UART) || defined(CONFIG_LOGGER_USB)
        Logger(static_cast<Logger::Level>(CONFIG_LOGGER_LEVEL), Logger::Color_mode::Prefix);
        #ifdef CONFIG_LOGGER_UART
            Logger::Init_UART(uart0, 0, 1, 961200);
        #endif
        #ifdef CONFIG_LOGGER_USB
            Logger::Init_USB(1);
        #endif
        Logger::Print_raw("\r\n");
        Logger::Critical("Device start");
        Logger::Notice("Logger UART Initialized");
        if(watchdog_enable_caused_reboot()){
            Logger::Error("Watchdog caused reboot");
        }
    #endif

    auto cli = new CLI_service();

    #ifdef CONFIG_CONTROL_MODULE
        new Control_module();
    #elifdef CONFIG_SENSOR_MODULE
        new Sensor_module();
    #elifdef CONFIG_PUMP_MODULE
        new Pump_module();
    #else
        #error "No module defined, use 'make menuconfig' to select module"
    #endif

    Base_module::Singleton_instance()->Connect_to_cli(*cli);

    new USB_thread();

    fra::Thread::StartScheduler();
}
