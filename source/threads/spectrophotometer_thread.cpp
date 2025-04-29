#include "spectrophotometer_thread.hpp"

Spectrophotometer_thread::Spectrophotometer_thread(Spectrophotometer * const spectrophotometer):
    Thread("spectrophotometer_thread", 2048, 7),
    spectrophotometer(spectrophotometer){
    Start();
}

void Spectrophotometer_thread::Run(){
    Logger::Trace("Spectrophotometer thread start");

    while (true) {

        // Locking mutex to prevent access to cuvette while measuring
        bool lock = spectrophotometer->cuvette_mutex->Lock(0);
        if (!lock) {
            Logger::Warning("Spectrophotometer waiting for cuvette access");
            spectrophotometer->cuvette_mutex->Lock();
        }
        Logger::Debug("Spectrophotometer cuvette access granted");

        while(not message_buffer.empty()){

            auto message = message_buffer.front();
            message_buffer.pop();

            auto message_type = message.Message_type();

            //Check if message type is supported
            if (std::find(supported_messages.begin(), supported_messages.end(), message_type) == supported_messages.end()){
                Logger::Error("Spectrophotometer thread does not support Message type: {}", Codes::to_string(message_type));
                continue;
            }

            switch(message_type){
                case Codes::Message_type::Spectrophotometer_measurement_request: {
                    Logger::Notice("Spectrophotometer measurement start");
                    App_messages::Spectrophotometer::Measurement_request request;
                    if (not request.Interpret_data(message.data)) {
                        Logger::Error("Failed to interpret spectrophotometer measurement request");
                        continue;
                    }

                    uint8_t channel_index = request.channel;

                    if (channel_index > spectrophotometer->channels.size()) {
                        Logger::Error("Requested spectrophotometer channel out of range");
                        continue;
                    }

                    Spectrophotometer::Channels channel_name = static_cast<Spectrophotometer::Channels>(request.channel);

                    Spectrophotometer::Measurement measurement = spectrophotometer->Measure_channel(channel_name);

                    App_messages::Spectrophotometer::Measurement_response response;
                    response.channel = static_cast<uint8_t>(measurement.channel);
                    response.relative_value = measurement.relative_value;
                    response.absolute_value = measurement.absolute_value;

                    Logger::Debug("Channel: {}, relative {:05.3f}, absolute {}",
                            (int)static_cast<short>(measurement.channel),
                            (float)measurement.relative_value,
                            (int)measurement.absolute_value
                            );

                    spectrophotometer->Send_CAN_message(response);
                } break;

                case Codes::Message_type::Spectrophotometer_calibrate: {
                    Logger::Notice("Spectrophotometer calibration started");
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
        Logger::Error("Message type {} not supported", Codes::to_string(message.Message_type()));
        return false;
    }

    if (message_buffer.full()){
        Logger::Error("Spectrophotometer thread message buffer full");
        return false;
    }

    message_buffer.push(message);
    // Resume thread to process message
    Resume();
    return true;
}
