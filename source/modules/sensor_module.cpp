#include "sensor_module.hpp"

Sensor_module::Sensor_module():
    Base_module(Codes::Module::Sensor_module, Codes::Instance::Exclusive, 24, 13),
    i2c(new I2C_bus(i2c1, 10, 11, 100000, true)),
    ntc_channel_selector(new GPIO(18, GPIO::Direction::Out)),
    ntc_thermistors(new Thermistor(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_3, 3.30f), 3950, 10000, 25, 5100))
{
    Setup_components();
}

void Sensor_module::Setup_components(){
    Logger::Print("Sensor module component setup", Logger::Level::Debug);
    Setup_bottle_thermometers();
    Setup_Mini_OLED();
    Setup_fluorometer();
}

float Sensor_module::Board_temperature(){
    ntc_channel_selector->Set(true);
    return ntc_thermistors->Temperature();
}

void Sensor_module::Setup_Mini_OLED(){
    Logger::Print("Setting up Mini OLED", Logger::Level::Debug);
    mini_oled = new Mini_OLED(bottle_temperature, 5);
}

void Sensor_module::Setup_bottle_thermometers(){
    Logger::Print("Setting up bottle thermometers", Logger::Level::Debug);

    TLA2024 *adc = new TLA2024(*i2c, 0x4b);

    TLA2024_channel *adc_ch0 = new TLA2024_channel(adc, TLA2024::Channels::AIN0_GND);
    TLA2024_channel *adc_ch1 = new TLA2024_channel(adc, TLA2024::Channels::AIN1_GND);
    TLA2024_channel *adc_ch2 = new TLA2024_channel(adc, TLA2024::Channels::AIN2_GND);
    TLA2024_channel *adc_ch3 = new TLA2024_channel(adc, TLA2024::Channels::AIN3_GND);

    Thermopile *thermopile_top    = new Thermopile(adc_ch1, adc_ch0, 0.95);
    Thermopile *thermopile_bottom = new Thermopile(adc_ch3, adc_ch2, 0.95);

    bottle_temperature = new Bottle_temperature(thermopile_top, thermopile_bottom);
}

void Sensor_module::Setup_fluorometer(){
    Logger::Print("Setting up fluorometer", Logger::Level::Debug);
    auto led_pwm = new PWM_channel(23, 1000, 0.0, true);
    uint detector_gain_selector_pin = 21;

    fluorometer = new Fluorometer(led_pwm, detector_gain_selector_pin, ntc_channel_selector, ntc_thermistors, i2c);
}
