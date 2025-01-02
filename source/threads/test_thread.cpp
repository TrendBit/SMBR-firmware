#include "test_thread.hpp"
#include "can_bus/can_message.hpp"
#include "can_bus/message_router.hpp"
#include "modules/base_module.hpp"
#include "modules/control_module.hpp"

#include "components/led/led_pwm.hpp"
#include "components/led_panel.hpp"
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

    Test_temps();
};

void Test_thread::Test_RPM(){
    PWM * mix_fan_pwm = new PWM(13, 50, 0.5, true);
    auto mix_rpm_counter = new RPM_counter_PIO(PIO_machine(pio1,1),7, 100000.0,0.1,2);

    while (true) {

        DelayUntil(fra::Ticks::MsToTicks(1000));

        // Signal stretching in order to get clean RPM signal
        mix_fan_pwm->Duty_cycle(1.0);
        rtos::Delay(200);
        Logger::Print(emio::format("RPM: {:.4f}", mix_rpm_counter->RPM()));
        mix_fan_pwm->Duty_cycle(0.5);
    }
}

void Test_thread::Test_heater(){
    GPIO * heater_fan = new GPIO(11, GPIO::Direction::Out);
    heater_fan->Set(true);

    GPIO * heater_vref = new GPIO(20, GPIO::Direction::Out);
    heater_vref->Set(true);

    // heater = new Heater(23, 25, 10000);
    // 8W power (frequency 100 kHz): cooling -0.77
    // heater->Intensity(-0.3);

    float frequency = 100000;
    float intensity = -0.7;

    if (intensity > 0) {
        auto in1 = new PWM(23, frequency, intensity, true);
        auto in2 = new PWM(25, frequency, 0.0, true);
    } else {
        auto in1 = new PWM(23, frequency, 0.0, true);
        auto in2 = new PWM(25, frequency, -intensity, true);
    }
}

void Test_thread::Test_motors(){
    PWM * pump_in1 = new PWM(22, 50, 0.00, true);
    PWM * pump_in2 = new PWM( 8, 50, 0.00, true);

    DC_HBridge * pump = new DC_HBridge(pump_in1, pump_in2);
    pump->Speed(0.5);

    PWM * air_in1 = new PWM(3, 50, 0.00, true);
    PWM * air_in2 = new PWM(2, 50, 0.00, true);

    DC_HBridge * air = new DC_HBridge(air_in1, air_in2);
    air->Speed(0.25);
}


void Test_thread::Test_temps(){
    RP_internal_temperature * mcu_internal_temp = new RP_internal_temperature(3.30f);

    auto temp_b_adc = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_1, 3.30f);
    auto temp_b = new Thermistor(temp_b_adc, 3950, 10000, 25, 5100);

    auto temp_0_adc = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_2, 3.30f);
    auto temp_0 = new Thermistor(temp_0_adc, 4190, 100000, 25, 5100);

    auto temp_1_adc = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_3, 3.30f);
    auto temp_1 = new Thermistor(temp_1_adc, 3950, 100000, 25, 30000);

    rtos::Delay(100);

    while (true) {

        DelayUntil(fra::Ticks::MsToTicks(1000));

        Logger::Print(emio::format("   Bottle temperature: {:05.2f}°C", temp_0->Temperature()));
        Logger::Print(emio::format("   Heater temperature: {:05.2f}°C", temp_1->Temperature()));
        Logger::Print(emio::format("    Board temperature: {:05.2f}°C", temp_b->Temperature()));
        Logger::Print(emio::format("      MCU temperature: {:05.2f}°C", mcu_internal_temp->Temperature()));
        Logger::Print("-------");

    }
}
