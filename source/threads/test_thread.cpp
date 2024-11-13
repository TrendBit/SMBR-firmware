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

    PWM * mix_fan_pwm = new PWM(13, 2000, 0.00, true);
    auto mix_rpm_counter = new RPM_counter_PIO(PIO_machine(pio1,1),7, 10000.0,1.0,2);

    Fan_RPM * mix_fan = new Fan_RPM(mix_fan_pwm, mix_rpm_counter);
    mix_fan->Intensity(0.0);

    PWM * case_fan_pwm = new PWM(12, 2000, 0.00, true);
    auto case_rpm_counter = new RPM_counter_PIO(PIO_machine(pio1,0),9, 10000.0,1.0,2);

    Fan_RPM * case_fan = new Fan_RPM(case_fan_pwm, case_rpm_counter);
    case_fan->Intensity(1.0);

    PWM * heat_fan_pwm = new PWM(11, 2000, 0.00, true);
    auto heat_rpm_counter = new RPM_counter_PIO(PIO_machine(pio1,0),8, 10000.0,1.0,2);

    Fan_RPM * heat_fan = new Fan_RPM(heat_fan_pwm, heat_rpm_counter);
    heat_fan->Intensity(1.0);

    PWM * pump_in1 = new PWM(22, 50, 0.00, true);
    PWM * pump_in2 = new PWM( 8, 50, 0.00, true);

    DC_HBridge * pump = new DC_HBridge(pump_in1, pump_in2);
    pump->Speed(0.0);

    PWM * air_in1 = new PWM(3, 50, 0.00, true);
    PWM * air_in2 = new PWM(2, 50, 0.00, true);

    DC_HBridge * air = new DC_HBridge(air_in1, air_in2);
    air->Speed(0.0);

    PWM * heater_in1 = new PWM(23, 50, 0.00, true);
    PWM * heater_in2 = new PWM(25, 50, 0.00, true);

    DC_HBridge * heater = new DC_HBridge(heater_in1, heater_in2);
    heater->Speed(0.0);

    auto temp_0_adc = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_2, 3.30f);
    auto temp_0 = new Thermistor(temp_0_adc, 3950, 10000, 25, 5100);

    auto temp_1_adc = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_3, 3.30f);
    auto temp_1 = new Thermistor(temp_1_adc, 3435, 10000, 25, 5100);

    while (true) {

        //DelayUntil(fra::Ticks::MsToTicks(1000));

        rtos::Delay(5000);

        air->Speed(1.0);

        rtos::Delay(3000);

        air->Speed(0.0);

        rtos::Delay(500);

        pump->Speed(1.0);

        rtos::Delay(3000);

        pump->Speed(0.0);

        rtos::Delay(500);

        mix_fan->Intensity(0.5);

        rtos::Delay(3000);

        mix_fan->Intensity(0.0);

        for (int led_channel = 0; led_channel <= 3;  led_channel++) {

            for(float led_intensity = 0.0f; led_intensity <= 1.0f; led_intensity += 0.1f){
                if (led_illumination_global) {
                    led_illumination_global->Set_intensity(led_channel, led_intensity);
                }
                rtos::Delay(250);
            }

            if (led_illumination_global) {
                led_illumination_global->Set_intensity(led_channel, 0.0f);
            }

            rtos::Delay(1000);

        }

        for(float led_intensity = 0.0f; led_intensity <= 1.0f; led_intensity += 0.1f){
            if (led_illumination_global) {
                for (int led_channel = 0; led_channel <= 4;  led_channel++) {
                    led_illumination_global->Set_intensity(led_channel, led_intensity);
                }
            }
            rtos::Delay(250);
        }

        if (led_illumination_global) {
            for (int led_channel = 0; led_channel <= 4;  led_channel++) {
                led_illumination_global->Set_intensity(led_channel, 0.0);
            }
        }

        //Logger::Print(emio::format("RPM: {:.4f}", rpm_counter->RPM()));
        // Logger::Print(emio::format("TEMP0(LED): {:05.2f}°C, TEMP1:{:05.2f}°C, Fan: {:07.2f} RPM", temp_0->Temperature(), temp_1->Temperature(), mix_fan->RPM()));
        //Logger::Print(emio::format("RPM: {}", std::numeric_limits<uint32_t>::max() - rpm_counter->Read_PIO_counter()));
    }

    /*
    new Control_module();

    core = new Common_core(can_thread);

    PWM * air1 = new PWM(6, 200, 0.0, true);
    //GPIO * air_pump_in1 = new GPIO(6, GPIO::Direction::Out);
    GPIO * air_pump_in2 = new GPIO(2, GPIO::Direction::Out);
    //air_pump_in1->Set(true);
    air_pump_in2->Set(false);

    PWM * pelt_in1 = new PWM(24, 200, 0.0, true);

    //GPIO * pelt_in1 = new GPIO(24, GPIO::Direction::Out);
    GPIO * pelt_in2 = new GPIO(25, GPIO::Direction::Out);
    //pelt_in1->Set(true);
    pelt_in2->Set(false);

    PWM * fan = new PWM(11, 2000, 0.00, true);

    auto temp_0_adc = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_2, 3.30f);
    auto temp_0 = new Thermistor(temp_0_adc, 3435, 10000, 25, 5100);

    auto temp_1_adc = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_3, 3.30f);
    auto temp_1 = new Thermistor(temp_1_adc, 3435, 10000, 25, 5100);

    while (true) {
        // Logger::Print(emio::format(",{:05.2f},{:05.2f}", temp_0->Temperature(), temp_1->Temperature()));

        DelayUntil(fra::Ticks::MsToTicks(1000));

        if (can_thread->Message_available()) {
            CAN::Message message_in = can_thread->Read_message().value();
            Logger::Print("Message available");
            Message_router::Route(message_in);
        }
    }*/
};
