#include "main.hpp"
#include "threads/test_thread.hpp"


namespace fra = cpp_freertos;

int main(){
    timer_hw->dbgpause = 0; // Required for SWD debug otherwise timers are alway zero during debug

    Logger::Init_UART();

    CAN_thread * can_thread = new CAN_thread();
    new LED_heartbeat_thread(12, 500);
    new USB_thread();
    new CLI_service();
    new Test_thread(can_thread);

    fra::Thread::StartScheduler();

}  // main



