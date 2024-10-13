#include "test_thread.hpp"
#include "can_bus/can_message.hpp"
#include "can_bus/message_router.hpp"
#include "modules/base_module.hpp"
#include "modules/control_module.hpp"

#include "components/led/led_pwm.hpp"
#include "components/led_illumination.hpp"
#include "components/fan/fan_gpio.hpp"
#include "components/fan/fan_pwm.hpp"
#include "components/fan/fan_rpm.hpp"
#include "components/motors/dc_hbridge.hpp"

#include "hardware/pwm.h"

#include "logger.hpp"
#include "emio/emio.hpp"

Test_thread::Test_thread(CAN_thread * can_thread)
    : Thread("test_thread", 2048, 10),
    can_thread(can_thread)
    {
    Start();
};

Test_thread::Test_thread()
    : Thread("test_thread", 2048, 10)
    {
    Start();
};

void Test_thread::Run(){

    Logger::Print("Test thread init");

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1000));

    }
};
