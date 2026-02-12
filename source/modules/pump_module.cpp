#include "pump_module.hpp"

Pump_module::Pump_module():
    Base_module(
        Codes::Module::Pump_module,
        new Enumerator(
            Codes::Module::Pump_module,
            memory,
            Codes::Instance::Undefined,
            enumeration_button_gpio,
            enumeration_rgb_led_gpio),
        activity_green_led_gpio, i2c_sda_gpio, i2c_scl_gpio),
    board_thermistor(new Thermistor(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_1, 3.30f), 3950, 10000, 25, 5100))
{
    Setup_components();
}

void Pump_module::Setup_components(){
    Logger::Debug("Pump module component setup");

    uint8_t pump_count;
    // Detect number of pumps based on configuration pin
    GPIO config_pin = GPIO(config_detection_pin);
    config_pin.Set_direction(GPIO::Direction::In);
    config_pin.Set_pulls(true, false);
    if (config_pin.Read()) {
        pump_count = 4;
    } else {
        pump_count = 2;
    }

    Logger::Notice("Module configuration: {} pumps", pump_count);

    auto pump_adc = new TLA2024(*i2c, 0x48);

    auto adc_channel_1 = new TLA2024_channel(pump_adc, TLA2024::Channels::AIN0_GND);
    auto adc_channel_2 = new TLA2024_channel(pump_adc, TLA2024::Channels::AIN1_GND);
    auto adc_channel_3 = new TLA2024_channel(pump_adc, TLA2024::Channels::AIN2_GND);
    auto adc_channel_4 = new TLA2024_channel(pump_adc, TLA2024::Channels::AIN3_GND);

    auto current_sensor_1 = std::make_unique<Current_sensor>(adc_channel_1, 0.1f);
    auto current_sensor_2 = std::make_unique<Current_sensor>(adc_channel_2, 0.1f);
    auto current_sensor_3 = std::make_unique<Current_sensor>(adc_channel_3, 0.1f);
    auto current_sensor_4 = std::make_unique<Current_sensor>(adc_channel_4, 0.1f);

    // TODO, some pumps must be inverted in order to have correct direction (based on device case)
    auto pump_1 = new Pump(15, 14, 16, std::move(current_sensor_1), 1000.0f, 0.1f);
    auto pump_2 = new Pump(11, 10, 17, std::move(current_sensor_2), 1000.0f, 0.1f);
    auto pump_3 = new Pump( 7,  6, 20, std::move(current_sensor_3), 1000.0f, 0.1f);
    auto pump_4 = new Pump( 2,  3, 21, std::move(current_sensor_4), 1000.0f, 0.1f);

    pump_controller = new Pump_controller(
        pump_count == 2 ?
            etl::vector<Pump *,8>{ pump_1, pump_2 } :
            etl::vector<Pump *,8>{ pump_1, pump_2, pump_3, pump_4 },
        memory
    );

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
