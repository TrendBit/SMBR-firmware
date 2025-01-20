#include "sensor_module.hpp"

Sensor_module::Sensor_module():
    Base_module(Codes::Module::Sensor_module, Codes::Instance::Exclusive, 24, 13)
{
    Setup_components();
}

void Sensor_module::Setup_components(){
    Logger::Print("Sensor module component setup");
    Setup_Mini_OLED();
}

float Sensor_module::Board_temperature(){
    return std::numeric_limits<float>::quiet_NaN();
}

void Sensor_module::Setup_Mini_OLED(){
    Logger::Print("Setting up Mini OLED");
    mini_oled = new Mini_OLED(5);
}
