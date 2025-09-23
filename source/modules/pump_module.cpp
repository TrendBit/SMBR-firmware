#include "pump_module.hpp"

Pump_module::Pump_module():
    Base_module(Codes::Module::Pump_module, new Enumerator(Codes::Module::Pump_module,memory,Codes::Instance::Undefined, 22, 23), 24, 18, 19),
    board_thermistor(new Thermistor(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_1, 3.30f), 3950, 10000, 25, 5100))
{
    Setup_components();
}

void Pump_module::Setup_components(){
    Logger::Debug("Pump module component setup");
    Logger::Warning("No components to setup");
}

std::optional<float> Pump_module::Board_temperature(){
    bool lock = adc_mutex->Lock(0);
    if (!lock) {
        Logger::Warning("Board temp ADC mutex lock failed");
        return std::nullopt;
    }
    float temp = board_thermistor->Temperature();
    adc_mutex->Unlock();
    return temp;
}
