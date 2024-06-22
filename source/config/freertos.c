#include <stdint.h>
#include "hardware/timer.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#ifdef __GNUC__
#define USED __attribute__((used))
#else
#define USED
#endif

const int USED uxTopUsedPriority = configMAX_PRIORITIES - 1;

void configureTimerForRunTimeStats(void){
}

volatile unsigned long getRunTimeCounterValue(void){
    uint32_t time = time_us_64()/1000;
    return time;
}

/* FreeRTOS callbacks */
void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );

void vApplicationMallocFailedHook(void){
    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName){
    (void)pxTask;
    (void)pcTaskName;
    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}
