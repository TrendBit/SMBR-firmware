#include "sensor_module.hpp"
#include "threads/module_check_thread.hpp"
#include "module_check/board_temperature_check.hpp"
#include "module_check/core_temperature_check.hpp"
#include "module_check/core_load_check.hpp"
#include "module_check/bottle_temp_check.hpp"
#include "module_check/bottle_top_measured_temp_check.hpp"

Sensor_module::Sensor_module():
    Base_module(
        Codes::Module::Sensor_module,
        new Enumerator(Codes::Instance::Exclusive),
        24, 10, 11, 13),
    ntc_channel_selector(new GPIO(18, GPIO::Direction::Out)),
    ntc_thermistors(new Thermistor(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_3, 3.30f), 3950, 10000, 25, 5100)),
    cuvette_mutex(new fra::MutexStandard())
{
    Setup_components();
}

void Sensor_module::Setup_components(){
    Logger::Debug("Sensor module component setup");
    Setup_bottle_thermometers();
    Setup_Mini_OLED();
    Setup_fluorometer();
    Setup_spectrophotometer();
    Setup_module_check();
}

std::optional<float> Sensor_module::Board_temperature(){
    bool lock = adc_mutex->Lock(0);
    if (!lock) {
        Logger::Warning("Board temp ADC mutex lock failed");
        return std::nullopt;
    }
    ntc_channel_selector->Set(true);
    float temp = ntc_thermistors->Temperature();
    adc_mutex->Unlock();
    return temp;
}

void Sensor_module::Setup_Mini_OLED(){
    Logger::Debug("Setting up Mini OLED");
    mini_oled = new Mini_OLED(bottle_temperature, 5);
}

void Sensor_module::Setup_bottle_thermometers(){
    Logger::Debug("Setting up bottle thermometers");

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
    Logger::Debug("Setting up fluorometer");
    auto led_pwm = new PWM_channel(23, 1000000, 0.0, true);
    uint detector_gain_selector_pin = 21;

    fluorometer = new Fluorometer(led_pwm, detector_gain_selector_pin, ntc_channel_selector, ntc_thermistors, i2c, memory, cuvette_mutex, adc_mutex);
}

void Sensor_module::Setup_spectrophotometer(){
    spectrophotometer = new Spectrophotometer(*i2c, memory, cuvette_mutex);
}

void Sensor_module::Setup_module_check(){
    module_check_thread->AttachCheck(new Board_temperature_check(this));
    module_check_thread->AttachCheck(new Core_temperature_check(common_core));
    module_check_thread->AttachCheck(new Core_load_check(common_core));
    if (bottle_temperature) {
        module_check_thread->AttachCheck(new Bottle_temp_check(bottle_temperature));
        module_check_thread->AttachCheck(new Bottle_top_measured_temp_check(bottle_temperature));
    }
}