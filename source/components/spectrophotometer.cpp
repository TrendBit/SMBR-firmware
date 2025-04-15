#include "spectrophotometer.hpp"

Spectrophotometer::Spectrophotometer(I2C_bus &i2c):
    Component(Codes::Component::Spectrophotometer),
    Message_receiver(Codes::Component::Spectrophotometer),
    light_sensor(new VEML6040(i2c, 0x10)),
    drivers({new KTD2026(i2c, 0x31), new KTD2026(i2c, 0x30)}),
    temperature_sensor(new TMP102(i2c, 0x49))
{
    drivers[0]->Init();
    drivers[1]->Init();
}

uint16_t Spectrophotometer::Read_detector_raw(Channels channel){
    return light_sensor->Measure(channels.at(channel).sensor_channel);
}

float Spectrophotometer::Read_detector(Channels channel){
    return light_sensor->Measure_relative(channels.at(channel).sensor_channel);
}

float Spectrophotometer::Measure_relative(Channels channel){
    float emitor_intensity = channels.at(channel).intensity_compensation;
    VEML6040::Exposure exposure_time = channels.at(channel).exposure_time;
    light_sensor->Exposure_time(exposure_time);
    Set(channel, emitor_intensity);
    uint delay = VEML6040::Measurement_time(exposure_time);
    rtos::Delay(2000);
    float detector_value = light_sensor->Measure_relative(channels.at(channel).sensor_channel);
    float multiplier = channels.at(channel).multiplier;
    Logger::Print(emio::format("Channel: {}", static_cast<uint8_t>(channel)), Logger::Level::Debug);
    Logger::Print(emio::format("Detector value: {:05.3f}", detector_value), Logger::Level::Debug);
    Logger::Print(emio::format("Exposure time: {:d} ms", delay), Logger::Level::Debug);
    Logger::Print(emio::format("Emitor intensity: {:05.3f}", emitor_intensity), Logger::Level::Debug);
    Logger::Print(emio::format("Multiplier: {:05.3f}", multiplier), Logger::Level::Debug);
    detector_value *= multiplier;
    Set(channel, 0.0f);
    return detector_value;
}

bool Spectrophotometer::Set(Channels channel, float intensity){
    KTD2026 *driver = drivers[channels.at(channel).driver_instance];
    intensity = std::clamp(intensity, 0.0f, 1.0f);
    driver->Intensity(channels.at(channel).driver_channel, intensity);
    return true;
}

float Spectrophotometer::Temperature(){
    return temperature_sensor->Temperature();
}

void Spectrophotometer::Calibrate_channels(){

    Logger::Print("Base line", Logger::Level::Notice);
    light_sensor->Exposure_time(VEML6040::Exposure::_160_ms);
    Set(Channels::UV, 0.0);
    Set(Channels::Blue, 1.0);
    Set(Channels::Green, 0.0);
    Set(Channels::Orange, 0.0);
    Set(Channels::Red, 0.0);
    Set(Channels::IR, 0.0);
    rtos::Delay(1000);
    Logger::Print(emio::format("Red: {:05.3f}", light_sensor->Measure_relative(VEML6040::Channels::Red)));
    Logger::Print(emio::format("Green: {:05.3f}", light_sensor->Measure_relative(VEML6040::Channels::Green)));
    Logger::Print(emio::format("Blue: {:05.3f}", light_sensor->Measure_relative(VEML6040::Channels::Blue)));
    Logger::Print(emio::format("White: {:05.3f}", light_sensor->Measure_relative(VEML6040::Channels::White)));

    std::unordered_map<Channels, float> intensity_compensation = {};

    etl::array<Channels,6> channels_to_calibrate = {
        Channels::UV,
        Channels::Blue,
        Channels::Green,
        Channels::Orange,
        Channels::Red,
        Channels::IR,
    };

    for (auto channel : channels_to_calibrate) {
        intensity_compensation[channel] = channels.at(channel).intensity_compensation;
    }

    for (auto channel : channels_to_calibrate) {
        VEML6040::Exposure exposure_time = channels.at(channel).exposure_time;
        float intensity = channels.at(channel).intensity_compensation;
        float measurement_time = light_sensor->Exposure_time(exposure_time);
        Set(channel, intensity);
        rtos::Delay(2000);
        intensity_compensation[channel] = Read_detector(channel);
        Logger::Print(emio::format("Intensity: {:05.3f}", intensity_compensation[channel]));
        Set(channel, 0.0);
    }

    float min_intensity = std::max_element(intensity_compensation.begin(), intensity_compensation.end(), [](auto a, auto b){ return a.second > b.second; })->second;
    float max_intensity = std::min_element(intensity_compensation.begin(), intensity_compensation.end(), [](auto a, auto b){ return a.second < b.second; })->second;

    Logger::Print(emio::format("Min intensity: {:05.3f}", max_intensity));
    Logger::Print(emio::format("Max intensity: {:05.3f}", max_intensity));

    for (auto channel : channels_to_calibrate) {
        intensity_compensation[channel] = min_intensity / intensity_compensation[channel];
        Logger::Print(emio::format("Compensation: {:05.3f}", intensity_compensation[channel]));
        channels[channel].intensity_compensation = intensity_compensation[channel];
    }
}

bool Spectrophotometer::Receive(Application_message message){
    switch (message.Message_type()) {
        case Codes::Message_type::Spectrophotometer_channel_count_request: {
            Logger::Print("Spectrophotometer channel count request", Logger::Level::Notice);
            App_messages::Spectrophotometer::Channel_count_response response;
            response.channel_count = channels.size();
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Spectrophotometer_channel_info_request: {
            Logger::Print("Spectrophotometer channel info request", Logger::Level::Notice);
            App_messages::Spectrophotometer::Channel_info_request request;
            if (not request.Interpret_data(message.data)) {
                Logger::Print("Failed to interpret channel info request", Logger::Level::Error);
                return false;
            }

            uint8_t channel_index = request.channel;

            if (channel_index > channels.size()) {
                Logger::Print("Requested channel out of range", Logger::Level::Error);
                return false;
            }

            Channels channel_name = static_cast<Channels>(request.channel);
            Channel channel_info = channels.at(channel_name);

            App_messages::Spectrophotometer::Channel_info_response response;
            response.channel = channel_index;
            response.central_wavelength = channel_info.central_wavelength;
            response.half_sensitivity_width = channel_info.half_sensitivity_width;

            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Spectrophotometer_measurement_request: {
            Logger::Print("Spectrophotometer measurement request", Logger::Level::Notice);
            App_messages::Spectrophotometer::Measurement_request request;
            if (not request.Interpret_data(message.data)) {
                Logger::Print("Failed to interpret measurement request", Logger::Level::Error);
                return false;
            }

            uint8_t channel_index = request.channel;

            if (channel_index > channels.size()) {
                Logger::Print("Requested channel out of range", Logger::Level::Error);
                return false;
            }

            Channels channel_name = static_cast<Channels>(request.channel);

            float value = Measure_relative(channel_name);

            App_messages::Spectrophotometer::Measurement_response response;
            response.channel = channel_index;
            response.value = value;

            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Spectrophotometer_calibrate: {
            Logger::Print("Spectrophotometer calibration request", Logger::Level::Notice);
            Calibrate_channels();
            return true;
        }

        case Codes::Message_type::Spectrophotometer_temperature_request: {
            Logger::Print("Spectrophotometer temperature request", Logger::Level::Notice);
            App_messages::Spectrophotometer::Temperature_response response;
            response.temperature = Temperature();
            Send_CAN_message(response);
            return true;
        }

        default:
            return false;
    }
}

bool Spectrophotometer::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}
