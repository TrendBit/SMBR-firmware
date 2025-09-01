#include "threads/system_check_thread.hpp"
#include "system_check/led_temperature_check.hpp"
#include "system_check/board_temperature_check.hpp"

System_check_thread::System_check_thread(Base_module* module, LED_panel* panel)
    : Thread("system_check_thread", 2048, 5),
      board_check(new Board_temperature_check(module)),
      led_check(panel ? new Led_temperature_check(panel) : nullptr)
{
    Start();
}

void System_check_thread::Run() {
    while (true) {
        if (board_check) board_check->Run_check();
        if (led_check)   led_check->Run_check();
        DelayUntil(pdMS_TO_TICKS(5000));
    }
}

void System_check_thread::AttachLedPanel(LED_panel* panel) {
    if (panel && !led_check) {
        led_check = new Led_temperature_check(panel);
    }
}