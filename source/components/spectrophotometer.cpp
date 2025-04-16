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
    light_sensor->Mode_set(VEML6040::Mode::Trigger);
    light_sensor->Exposure_time(VEML6040::Exposure::_40_ms);
}

uint16_t Spectrophotometer::Read_detector_raw(Channels channel){
    return light_sensor->Measure(channels.at(channel).sensor_channel);
}

float Spectrophotometer::Read_detector(Channels channel){
    return light_sensor->Measure_relative(channels.at(channel).sensor_channel);
}

float Spectrophotometer::Measure_intensity(Channels channel){
    float emitor_intensity = channels.at(channel).emitter_intensity;
    VEML6040::Exposure exposure_time = channels.at(channel).exposure_time;
    uint delay = VEML6040::Measurement_time(exposure_time);

    light_sensor->Disable();
    light_sensor->Exposure_time(exposure_time);
    Set(channel, emitor_intensity);
    light_sensor->Enable();
    light_sensor->Trigger_now();

    rtos::Delay(delay*1.1);

    float detector_value = light_sensor->Measure_relative(channels.at(channel).sensor_channel);
    Set(channel, 0.0f);

    return detector_value;
}

Spectrophotometer::Measurement Spectrophotometer::Measure_channel(Channels channel){
    Measurement measurement;
    measurement.channel = channel;

    float intensity = Measure_intensity(channel);

    measurement.relative_value = Calculate_relative(channel, intensity);
    measurement.absolute_value = Calculate_absolute(channel, intensity);

    return measurement;
}

float Spectrophotometer::Calculate_relative(Channels channel, float intensity){
    float nominal_detection = channels.at(channel).nominal_detection;
    return intensity / nominal_detection;
}

uint16_t Spectrophotometer::Calculate_absolute(Channels channel, float intensity){
    float emitor_compensated = intensity * 1024.0f * (1.0f / channels.at(channel).emitter_intensity);
    uint exposure_time = VEML6040::Measurement_time(channels.at(channel).exposure_time);
    float exposure_compensated = emitor_compensated * (1280.0f / exposure_time);
    return exposure_compensated;
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

    std::unordered_map<Channels, float> channel_intensity = {};

    etl::array<Channels,6> channels_to_calibrate = {
        Channels::UV,
        Channels::Blue,
        Channels::Green,
        Channels::Orange,
        Channels::Red,
        Channels::IR,
    };

    Logger::Print("Spectrometer calibration in progress", Logger::Level::Debug);

    for (auto channel : channels_to_calibrate) {
        float detected_intensity = Measure_intensity(channel);
        Logger::Print(emio::format("Nominal intensity: {:05.3f}", detected_intensity), Logger::Level::Trace);
        channels[channel].nominal_detection = detected_intensity;
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

            Measurement measurement = Measure_channel(channel_name);

            App_messages::Spectrophotometer::Measurement_response response;
            response.channel = static_cast<uint8_t>(measurement.channel);
            response.relative_value = measurement.relative_value;
            response.absolute_value = measurement.absolute_value;

            Logger::Print(emio::format("Channel: {}, relative {:05.3f}, absolute {}",
                    static_cast<uint8_t>(measurement.channel),
                    measurement.relative_value,
                    measurement.absolute_value
                    ),
                Logger::Level::Debug);

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
