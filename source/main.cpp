#include "main.hpp"

namespace fra = cpp_freertos;

int main(){
    timer_hw->dbgpause = 0; // Required for SWD debug otherwise timers are alway zero during debug

    #ifdef CONFIG_LOGGER
        Logger::Init_UART(uart0, 0, 1, 961200);
        Logger::Print_raw("\r\n");
        Logger::Print("Logger UART Initialized");
    #endif

    new USB_thread();
    new CLI_service();


    #ifdef CONFIG_CONTROL_MODULE
        new Control_module();
    #elifdef CONFIG_SENSOR_MODULE
        new Sensor_module();
    #else
        #error "No module defined, use 'make menuconfig' to select module"
    #endif

    fra::Thread::StartScheduler();

}  // main



