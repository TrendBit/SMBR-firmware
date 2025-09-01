#include "threads/system_check_thread.hpp"
#include "system_check/led_temperature_check.hpp"
#include "system_check/board_temperature_check.hpp"

System_check_thread::System_check_thread()
    : Thread("system_check_thread", 2048, 5)
{
    Start();
}

void System_check_thread::AttachCheck(ISystemCheck* check) {
    if (check) {
        checks.push_back(check);
    }
}

void System_check_thread::Run() {
    while (true) {
        for (auto* check : checks) {
            if (check) {
                check->Run_check();
            }
        }
        DelayUntil(pdMS_TO_TICKS(5000));
    }
}