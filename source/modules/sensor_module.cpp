#include "sensor_module.hpp"

Sensor_module::Sensor_module():
    Base_module(Codes::Module::Sensor_module, Codes::Instance::Exclusive, 12, 13)
{
    Setup_components();
}

void Sensor_module::Setup_components(){
    Logger::Print("Sensor module component setup");
}

float Sensor_module::Board_temperature(){
    return std::numeric_limits<float>::quiet_NaN();
}
