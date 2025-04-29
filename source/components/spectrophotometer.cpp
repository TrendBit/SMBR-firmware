#include "spectrophotometer.hpp"

#include "threads/spectrophotometer_thread.hpp"

Spectrophotometer::Spectrophotometer(I2C_bus &i2c, EEPROM_storage * const memory, fra::MutexStandard * cuvette_mutex):
    Component(Codes::Component::Spectrophotometer),
    Message_receiver(Codes::Component::Spectrophotometer),
    light_sensor(new VEML6040(i2c, 0x10)),
    drivers({new KTD2026(i2c, 0x31), new KTD2026(i2c, 0x30)}),
    temperature_sensor(new TMP102(i2c, 0x49)),
    memory(memory),
    spectrophotometer_thread(new Spectrophotometer_thread(this)),
    cuvette_mutex(cuvette_mutex)
{
    drivers[0]->Init();
    drivers[1]->Init();
    light_sensor->Mode_set(VEML6040::Mode::Trigger);
    light_sensor->Exposure_time(VEML6040::Exposure::_40_ms);
    Load_calibration();
}

bool Spectrophotometer::Load_calibration(){
    std::array<float, 6> nominal_calibration = {};
    bool status = memory->Read_spectrophotometer_calibration(nominal_calibration);

    if (status) {
        Logger::Debug("Spectrophotometer calibration data loaded from memory");
    } else {
        Logger::Error("Failed to load spectrophotometer calibration data from memory");
        return false;
    }

    for (size_t i = 0; i < channels.size(); ++i) {
        channels.at(static_cast<Channels>(i)).nominal_detection = nominal_calibration[i];
    }

    return true;
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

    Logger::Trace("Spectrophotometer calibration in progress");

    for (auto channel : channels_to_calibrate) {
        float detected_intensity = Measure_intensity(channel);
        Logger::Trace("Nominal intensity: {:05.3f}", detected_intensity);
        channels[channel].nominal_detection = detected_intensity;
    }

    std::array<float, 6> nominal_calibration = {
        channels[Channels::UV].nominal_detection,
        channels[Channels::Blue].nominal_detection,
        channels[Channels::Green].nominal_detection,
        channels[Channels::Orange].nominal_detection,
        channels[Channels::Red].nominal_detection,
        channels[Channels::IR].nominal_detection,
    };

    bool status = memory->Write_spectrophotometer_calibration(nominal_calibration);

    if (status) {
        Logger::Notice("Spectrophotometer calibration done, data written to memory");
    } else {
        Logger::Error("Failed to write spectrophotometer calibration data to memory");
    }
}

bool Spectrophotometer::Receive(Application_message message){
    switch (message.Message_type()) {
        case Codes::Message_type::Spectrophotometer_channel_count_request: {
            Logger::Notice("Spectrophotometer channel count request");
            App_messages::Spectrophotometer::Channel_count_response response;
            response.channel_count = channels.size();
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Spectrophotometer_channel_info_request: {
            Logger::Notice("Spectrophotometer channel info request");
            App_messages::Spectrophotometer::Channel_info_request request;
            if (not request.Interpret_data(message.data)) {
                Logger::Error("Failed to interpret channel info request");
                return false;
            }

            uint8_t channel_index = request.channel;

            if (channel_index > channels.size()) {
                Logger::Error("Requested channel out of range");
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
            Logger::Notice("Spectrophotometer measurement request enqueued");
            spectrophotometer_thread->Enqueue_message(message);
            return true;
        }

        case Codes::Message_type::Spectrophotometer_calibrate: {
            Logger::Notice("Spectrophotometer calibration request enqueued");
            spectrophotometer_thread->Enqueue_message(message);
            return true;
        }

        case Codes::Message_type::Spectrophotometer_temperature_request: {
            Logger::Notice("Spectrophotometer temperature request");
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
