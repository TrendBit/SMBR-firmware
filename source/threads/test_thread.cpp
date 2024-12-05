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

#include "rtos/delayed_execution.hpp"

#include "hal/i2c/i2c_bus.hpp"
#include "components/adc/TLA2024.hpp"
#include "components/adc/TLA2024_channel.hpp"
#include "components/thermometers/thermistor.hpp"
#include "components/thermometers/thermopile.hpp"
#include "components/led/KTD2026.hpp"
#include "components/photodetectors/VEML6040.hpp"
#include "components/memory/AT24Cxxx.hpp"
#include "components/thermometers/TMP102.hpp"

#include "logger.hpp"
#include "emio/emio.hpp"

Test_thread::Test_thread(CAN_thread *can_thread)
    : Thread("test_thread", 4096, 10),
    can_thread(can_thread){
    Start();
};

Test_thread::Test_thread()
    : Thread("test_thread", 4096, 10){
    Start();
};

void Test_thread::Run(){
    Logger::Print("Test thread init");

    I2C_bus *i2c = new I2C_bus(i2c1, 10, 11, 100000, true);

    Temp_sensor_test(*i2c);
};

void Test_thread::EEPROM_Test(I2C_bus &i2c){
    auto eeprom = new AT24Cxxx(i2c, 0x50, 256);

    auto read = eeprom->Read(0, 2);

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

    // 8W power (frequency 100 kHz): cooling -0.77
    Heater * heater = new Heater(23, 25, 100000);
    heater->Intensity(0.5);

/*
    float frequency = 100000;
    float intensity = -0.7;

    if (intensity > 0) {
        new PWM(23, frequency, intensity, true);
        new PWM(25, frequency, 0.0, true);
    } else {
        new PWM(23, frequency, 0.0, true);
        new PWM(25, frequency, -intensity, true);
    }*/
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

    auto lambda = [pump](){
        pump->Coast();
    };

    rtos::Delayed_execution *pump_stopper = new rtos::Delayed_execution(lambda);
    pump_stopper->Execute(2000);
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

    if(eeprom->Write(0, {0x01, 0x02})){
        Logger::Print("EEPROM write success");
    } else {
        Logger::Print("EEPROM write failed");
    }

    read = eeprom->Read(0, 2);

    if(read.has_value()){
        Logger::Print(emio::format("EEPROM read: 0x{:02x} 0x{:02x}", read.value()[0], read.value()[1]));
    } else {
        Logger::Print("EEPROM read failed");
    }
}

void Test_thread::Thermopile_test(I2C_bus &i2c){

    TLA2024 *adc = new TLA2024(i2c, 0x4b);

    TLA2024_channel *adc_ch0 = new TLA2024_channel(adc, TLA2024::Channels::AIN0_GND);
    TLA2024_channel *adc_ch1 = new TLA2024_channel(adc, TLA2024::Channels::AIN1_GND);
    TLA2024_channel *adc_ch2 = new TLA2024_channel(adc, TLA2024::Channels::AIN2_GND);
    TLA2024_channel *adc_ch3 = new TLA2024_channel(adc, TLA2024::Channels::AIN3_GND);

    Thermistor * thp_ntc   = new Thermistor(adc_ch0, 3960, 100000, 25, 100000, 3.3f);

    Thermopile * thermopile_top = new Thermopile(adc_ch1, adc_ch0, 0.95);
    Thermopile * thermopile_bottom = new Thermopile(adc_ch3, adc_ch2, 0.95);

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1000));

        Logger::Print(emio::format("Thermistor top    voltage: {:05.3f}V", adc_ch0->Read_voltage()));
        Logger::Print(emio::format("Thermopile top    voltage: {:05.3f}V", adc_ch1->Read_voltage()));
        Logger::Print(emio::format("Thermistor bottom voltage: {:05.3f}V", adc_ch2->Read_voltage()));
        Logger::Print(emio::format("Thermopile bottom voltage: {:05.3f}V", adc_ch3->Read_voltage()));
        Logger::Print("-------");
        Logger::Print(emio::format("Thermistor top    temperature: {:05.2f}°C", thermopile_top->Ambient()));
        Logger::Print(emio::format("Thermopile top    temperature: {:05.2f}°C", thermopile_top->Temperature()));
        Logger::Print(emio::format("Thermistor bottom temperature: {:05.2f}°C", thermopile_bottom->Ambient()));
        Logger::Print(emio::format("Thermopile bottom temperature: {:05.2f}°C", thermopile_bottom->Temperature()));
        Logger::Print("-------");
    }
}

void Test_thread::Fluoro_boost_test(){

    GPIO *LED_en = new GPIO(22, GPIO::Direction::Out);
    GPIO *LED_pwm = new GPIO(25, GPIO::Direction::Out);

    LED_en->Set(true);
    LED_pwm->Set(true);

    rtos::Delay(2000);

    LED_pwm->Set(false);
    LED_en->Set(false);
}

void Test_thread::Fluoro_buck_test(){

    auto led_pwm = new PWM(17, 100000, 0.0, true);
    led_pwm->Duty_cycle(1.0);

    rtos::Delay(2000);

    led_pwm->Duty_cycle(0.0);
}

void Test_thread::RGBW_sensor_test(I2C_bus &i2c){
    VEML6040 * rgb_sensor = new VEML6040(i2c, 0x10);

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1000));

        uint16_t det_red = rgb_sensor->Measure(VEML6040::channels::Red);
        rtos::Delay(10);
        uint16_t det_green = rgb_sensor->Measure(VEML6040::channels::Green);
        rtos::Delay(10);
        uint16_t det_blue = rgb_sensor->Measure(VEML6040::channels::Blue);
        rtos::Delay(10);
        uint16_t det_white = rgb_sensor->Measure(VEML6040::channels::White);
        rtos::Delay(10);

        Logger::Print(emio::format("RGBW: {:5d} {:5d} {:5d} {:5d}", det_red, det_green, det_blue, det_white));
    }

}

void Test_thread::OLED_test(){
    new Mini_display_thread();
}

void Test_thread::Temp_sensor_test(I2C_bus &i2c){
    GPIO *NTC_select = new GPIO(18, GPIO::Direction::Out);
    NTC_select->Set(true);

    auto ntc_adc = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_3, 3.30f);
    auto ntc     = new Thermistor(ntc_adc, 3950, 10000, 25, 5100);

    RP_internal_temperature * mcu_internal_temp = new RP_internal_temperature(3.30f);

    TMP102 * temp_1 = new TMP102(i2c, 0x48);
    TMP102 * temp_2 = new TMP102(i2c, 0x4a);
    TMP102 * temp_3 = new TMP102(i2c, 0x49);

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1000));

        Logger::Print(emio::format("Board temperature: {:05.2f}°C", ntc->Temperature()));
        Logger::Print(emio::format("MCU temperature: {:05.2f}°C", mcu_internal_temp->Temperature()));
        Logger::Print(emio::format("M2_inner temperature: {:05.2f}°C", temp_1->Temperature()));
        Logger::Print(emio::format("M2_outer temperature: {:05.2f}°C", temp_2->Temperature()));
        Logger::Print(emio::format("M3_driver temperature: {:05.2f}°C", temp_3->Temperature()));
        Logger::Print("-------");
    }
}

void Test_thread::LED_test(I2C_bus &i2c){
    KTD2026 * led_driver_0 = new KTD2026(i2c, 0x31);
    led_driver_0->Init();
    led_driver_0->Enable_channel(1);
    led_driver_0->Set_intensity(1, 20);
    led_driver_0->Enable_channel(2);
    led_driver_0->Set_intensity(2, 10);
    led_driver_0->Enable_channel(3);
    led_driver_0->Set_intensity(3, 10);

    KTD2026 * led_driver_1 = new KTD2026(i2c, 0x30);
    led_driver_1->Init();
    led_driver_1->Enable_channel(1);
    led_driver_1->Set_intensity(1, 10);
    led_driver_1->Enable_channel(2);
    led_driver_1->Set_intensity(2, 10);
    led_driver_1->Enable_channel(3);
    led_driver_1->Set_intensity(3, 10);
}

void Test_thread::Gain_detector_test(){
    GPIO * es_gain = new GPIO(21, GPIO::Direction::Out);
    es_gain->Set_pulls(false, false);
    es_gain->Set(true);

    GPIO * ts_gain = new GPIO(9, GPIO::Direction::Out);
    ts_gain->Set_pulls(false, false);
    ts_gain->Set(true);

    auto es_det = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_1, 3.30f);
    auto ts_det = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_2, 3.30f);

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1000));

        Logger::Print(emio::format("ES_VOLT: {:05.3f}V", es_det->Read_voltage()));
        Logger::Print(emio::format("TS_VOLT: {:05.3f}V", ts_det->Read_voltage()));
        Logger::Print("-------");
    }
}
