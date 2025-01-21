#include "sensor_module.hpp"

Sensor_module::Sensor_module():
    Base_module(Codes::Module::Sensor_module, Codes::Instance::Exclusive, 24, 13),
    i2c(new I2C_bus(i2c1, 10, 11, 100000, true))

{
    Setup_components();
}

void Sensor_module::Setup_components(){
    Logger::Print("Sensor module component setup");
    Setup_Mini_OLED();
    Setup_bottle_thermometers();
}

float Sensor_module::Board_temperature(){
    return std::numeric_limits<float>::quiet_NaN();
}

void Sensor_module::Setup_Mini_OLED(){
    Logger::Print("Setting up Mini OLED");
    mini_oled = new Mini_OLED(5);
}

void Sensor_module::Setup_bottle_thermometers(){
    Logger::Print("Setting up bottle thermometers");

    TLA2024 *adc = new TLA2024(*i2c, 0x4b);

    TLA2024_channel *adc_ch0 = new TLA2024_channel(adc, TLA2024::Channels::AIN0_GND);
    TLA2024_channel *adc_ch1 = new TLA2024_channel(adc, TLA2024::Channels::AIN1_GND);
    TLA2024_channel *adc_ch2 = new TLA2024_channel(adc, TLA2024::Channels::AIN2_GND);
    TLA2024_channel *adc_ch3 = new TLA2024_channel(adc, TLA2024::Channels::AIN3_GND);

    Thermopile *thermopile_top    = new Thermopile(adc_ch1, adc_ch0, 0.95);
    Thermopile *thermopile_bottom = new Thermopile(adc_ch3, adc_ch2, 0.95);

    bottle_temperature = new Bottle_temperature(thermopile_top, thermopile_bottom);
}
