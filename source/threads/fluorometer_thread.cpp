#include "fluorometer_thread.hpp"

Fluorometer_thread::Fluorometer_thread(Fluorometer * const fluorometer):
    Thread("fluorometer_thread", 2048, 7),
    fluorometer(fluorometer){
    Start();
}

void Fluorometer_thread::Run(){
    Logger::Print("Fluorometer thread start", Logger::Level::Trace);

    while (true) {

        // Lock adc mutex to prevent access of other component to ADC while measuring
        bool adc_lock = fluorometer->adc_mutex->Lock(0);
        if (!adc_lock) {
            Logger::Print("Fluorometer waiting for ADC access", Logger::Level::Warning);
            fluorometer->adc_mutex->Lock();
        }
        Logger::Print("Fluorometer ADC access granted", Logger::Level::Debug);

        // Locking cuvette mutex to prevent access of other component to cuvette while measuring
        bool cuvette_lock = fluorometer->cuvette_mutex->Lock(0);
        if (!cuvette_lock) {
            Logger::Print("Fluorometer waiting for cuvette access", Logger::Level::Warning);
            fluorometer->cuvette_mutex->Lock();
        }
        Logger::Print("Fluorometer cuvette access granted", Logger::Level::Debug);

        while(not message_buffer.empty()){

            auto message = message_buffer.front();
            message_buffer.pop();

            auto message_type = message.Message_type();

            //Check if message type is supported
            if (std::find(supported_messages.begin(), supported_messages.end(), message_type) == supported_messages.end()){
                Logger::Print(emio::format("Fluorometer thread does not support Message type: {}", Codes::to_string(message_type)), Logger::Level::Error);
                continue;
            }

            switch(message_type){
                case Codes::Message_type::Fluorometer_OJIP_capture_request: {
                    Logger::Print("Fluorometer OJIP capture start", Logger::Level::Notice);
                    App_messages::Fluorometer::OJIP_capture_request ojip_request;
                    ojip_request.Interpret_data(message.data);

                    if (not ojip_request.Interpret_data(message.data)) {
                        Logger::Print("Fluorometer OJIP Capture request interpretation failed", Logger::Level::Error);
                        break;
                    }

                    if (not fluorometer->ojip_capture_finished) {
                        Logger::Print("Fluorometer OJIP Capture in progress", Logger::Level::Warning);
                        break;
                    }

                    Logger::Print(emio::format("Starting capture with gain: {:2.0f}, intensity: {:04.2f}, length: {:3.1f}s, samples: {:d}",
                                                Fluorometer_config::gain_values.at(ojip_request.detector_gain),
                                                ojip_request.emitor_intensity,
                                                (ojip_request.length_ms/1000.0f),
                                                ojip_request.samples
                    ));

                    fluorometer->OJIP_data.measurement_id = ojip_request.measurement_id;
                    fluorometer->OJIP_data.emitor_intensity = ojip_request.emitor_intensity;

                    fluorometer->Capture_OJIP(ojip_request.detector_gain, ojip_request.emitor_intensity, (ojip_request.length_ms/1000.0f), ojip_request.samples, ojip_request.sample_timing);

                    fluorometer->OJIP_data.detector_gain = fluorometer->Gain();
                } break;

                case Codes::Message_type::Fluorometer_OJIP_retrieve_request: {
                    Logger::Print("Fluorometer OJIP export started", Logger::Level::Notice);
                    if(!fluorometer->ojip_capture_finished) {
                        Logger::Print("Fluorometer OJIP capture not finished", Logger::Level::Warning);
                        break;
                    }

                    if(fluorometer->OJIP_data.intensity.size() == 0) {
                        Logger::Print("Fluorometer OJIP data empty", Logger::Level::Warning);
                        break;
                    }

                    fluorometer->Export_data(&fluorometer->OJIP_data);
                } break;

                case Codes::Message_type::Fluorometer_calibration_request: {
                    Logger::Print("Fluorometer calibration request", Logger::Level::Notice);
                    fluorometer->Calibrate();
                } break;

                default:
                    break;
            }
        }
        // Unlocking mutex to allow other components to access cuvette
        fluorometer->cuvette_mutex->Unlock();
        fluorometer->adc_mutex->Unlock();

        // Suspend thread until new message is enqueued
        Suspend();
    }
}

bool Fluorometer_thread::Enqueue_message(Application_message &message){
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
