#include "threads/module_check_thread.hpp"
#include "module_check/led_temperature_check.hpp"
#include "module_check/board_temperature_check.hpp"

Module_check_thread::Module_check_thread()
    : Thread("module_check_thread", 2048, 5)
{
    Start();
}

void Module_check_thread::AttachCheck(IModuleCheck* check) {
    if (check) {
        checks.push_back(check);
    }
}

void Module_check_thread::Run() {
    while (true) {
        for (auto* check : checks) {
            if (check) {
                check->Run_check();
            }
        }
        DelayUntil(pdMS_TO_TICKS(5000));
    }
}