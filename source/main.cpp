#include "main.hpp"
#include "threads/test_thread.hpp"

namespace fra = cpp_freertos;

int main(){
    timer_hw->dbgpause = 0; // Required for SWD debug otherwise timers are alway zero during debug

    Logger::Init_UART();

    new LED_heartbeat_thread(12, 500);
    new USB_thread();
    new CLI_service();

    new Control_module();

    fra::Thread::StartScheduler();

}  // main



