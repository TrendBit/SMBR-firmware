#include "threads/system_check_thread.hpp"
#include "system_check/led_temperature_check.hpp"

System_check_thread::System_check_thread(LED_panel* panel)
    : Thread("system_check_thread", 2048, 5)
{
    temp_check = new Led_temperature_check(panel);
    Start();
}

void System_check_thread::Run() {
    while (true) {
        temp_check->Run_check();
        DelayUntil(pdMS_TO_TICKS(5000));
    }
}
