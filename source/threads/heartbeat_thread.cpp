#include "heartbeat_thread.hpp"

LED_heartbeat_thread::LED_heartbeat_thread(uint gpio_led_number, uint32_t delay)
    : Thread("heartbeat_thread", 1000, 8),
    led(new GPIO(gpio_led_number, GPIO::Direction::Out)),
    delay(delay){
    Start();
};

void LED_heartbeat_thread::Run(){
    led->Set(0);
    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(delay));
        led->Toggle();
    }
};
