#include "heartbeat_thread.hpp"
#include "hardware/watchdog.h"

Heartbeat_thread::Heartbeat_thread(uint gpio_led_number, uint32_t delay)
    : Thread("heartbeat_thread", 1000, 8),
    led(new GPIO(gpio_led_number, GPIO::Direction::Out)),
    delay(delay){
    Start();
};

void Heartbeat_thread::Run(){
    led->Set(0);
    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(delay));
        watchdog_update();
        led->Toggle();
    }
};
