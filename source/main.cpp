#include "hardware/structs/clocks.h"
#include "hardware/clocks.h"

#include "pico/time.h"
#include "pico/multicore.h"

#include "thread.hpp"

#include "threads/heartbeat_thread.hpp"

namespace fra = cpp_freertos;

#include "FreeRTOS.h"

#ifdef __GNUC__
#define USED __attribute__((used))
#else
#define USED
#endif

const int USED uxTopUsedPriority = configMAX_PRIORITIES - 1;

void main_task(void* params){
    while (1) {
        vTaskDelay(250);
        gpio_put(25, !gpio_get_out_level(25));
    }
}

void second_task(void* params){

    while (1) {
        vTaskDelay(220);
        gpio_put(25, !gpio_get_out_level(25));
    }
}

int main(){
    timer_hw->dbgpause = 0; // Required for SWD debug otherwise timers are alway zero during debug

    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 0);

    TaskHandle_t task_1;
    TaskHandle_t task_2;
    xTaskCreate(main_task, "MainThread", 2048, NULL, 5, &task_1);
    xTaskCreate(second_task, "SecThread", 2048, NULL, 5, &task_2);

    vTaskCoreAffinitySet(task_1, 0b01 );
    vTaskCoreAffinitySet(task_2, 0b10 );

    vTaskStartScheduler();

    // new LED_heartbeat_thread(25, 500);

    // fra::Thread::StartScheduler();

}  // main



