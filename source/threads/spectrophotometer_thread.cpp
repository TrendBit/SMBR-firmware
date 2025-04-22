#include "spectrophotometer_thread.hpp"

Spectrophotometer_thread::Spectrophotometer_thread(Spectrophotometer * const spectrophotometer):
    Thread("spectrophotometer_thread", 2048, 7),
    spectrophotometer(spectrophotometer){
    Start();
}

void Spectrophotometer_thread::Run(){
    Logger::Print("Spectrophotometer thread start", Logger::Level::Trace);

    while (true) {

        // Locking mutex to prevent access to cuvette while measuring
        bool lock = spectrophotometer->cuvette_mutex->Lock(0);
        if (!lock) {
            Logger::Print("Spectrophotometer waiting for cuvette access", Logger::Level::Warning);
            spectrophotometer->cuvette_mutex->Lock();
        }
        Logger::Print("Spectrophotometer cuvette access granted", Logger::Level::Debug);

        while(not message_buffer.empty()){

            auto message = message_buffer.front();
            message_buffer.pop();

            auto message_type = message.Message_type();

            //Check if message type is supported
            if (std::find(supported_messages.begin(), supported_messages.end(), message_type) == supported_messages.end()){
                Logger::Print(emio::format("Spectrophotometer thread does not support Message type: {}", Codes::to_string(message_type)), Logger::Level::Error);
                continue;
            }

            switch(message_type){
                case Codes::Message_type::Spectrophotometer_measurement_request: {
                    Logger::Print("Spectrophotometer measurement start", Logger::Level::Notice);
                    App_messages::Spectrophotometer::Measurement_request request;
                    if (not request.Interpret_data(message.data)) {
                        Logger::Print("Failed to interpret spectrophotometer measurement request", Logger::Level::Error);
                        continue;
                    }

                    uint8_t channel_index = request.channel;

                    if (channel_index > spectrophotometer->channels.size()) {
                        Logger::Print("Requested spectrophotometer channel out of range", Logger::Level::Error);
                        continue;
                    }

                    Spectrophotometer::Channels channel_name = static_cast<Spectrophotometer::Channels>(request.channel);

                    Spectrophotometer::Measurement measurement = spectrophotometer->Measure_channel(channel_name);

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

                    spectrophotometer->Send_CAN_message(response);
                } break;

                case Codes::Message_type::Spectrophotometer_calibrate: {
                    Logger::Print("Spectrophotometer calibration started", Logger::Level::Notice);
                    spectrophotometer->Calibrate_channels();
                } break;

                default:
                    break;
            }
        }
        // Unlocking mutex to allow other components to access cuvette
        spectrophotometer->cuvette_mutex->Unlock();

        // Suspend thread until new message is enqueued
        Suspend();
    }
}

bool Spectrophotometer_thread::Enqueue_message(Application_message &message){

    // Check if message type is supported
    if (std::find(supported_messages.begin(), supported_messages.end(), message.Message_type()) == supported_messages.end()){
        Logger::Print(emio::format("Message type {} not supported", Codes::to_string(message.Message_type())), Logger::Level::Error);
        return false;
    }

    if (message_buffer.full()){
        Logger::Print("Spectrophotometer thread message buffer full", Logger::Level::Error);
        return false;
    }

    message_buffer.push(message);
    // Resume thread to process message
    Resume();
    return true;
}
