#include "fluorometer_thread.hpp"

Fluorometer_thread::Fluorometer_thread(Fluorometer * const fluorometer):
    Thread("fluorometer_thread", 2048, 7),
    fluorometer(fluorometer){
    Start();
}

void Fluorometer_thread::Run(){
    Logger::Trace("Fluorometer thread start");

    while (true) {

        // Lock adc mutex to prevent access of other component to ADC while measuring
        bool adc_lock = fluorometer->adc_mutex->Lock(0);
        if (!adc_lock) {
            Logger::Warning("Fluorometer waiting for ADC access");
            fluorometer->adc_mutex->Lock();
        }
        Logger::Debug("Fluorometer ADC access granted");

        // Locking cuvette mutex to prevent access of other component to cuvette while measuring
        bool cuvette_lock = fluorometer->cuvette_mutex->Lock(0);
        if (!cuvette_lock) {
            Logger::Warning("Fluorometer waiting for cuvette access");
            fluorometer->cuvette_mutex->Lock();
        }
        Logger::Debug("Fluorometer cuvette access granted");

        while(not message_buffer.empty()){

            auto message = message_buffer.front();
            message_buffer.pop();

            auto message_type = message.Message_type();

            //Check if message type is supported
            if (std::find(supported_messages.begin(), supported_messages.end(), message_type) == supported_messages.end()){
                Logger::Error("Fluorometer thread does not support Message type: {}", Codes::to_string(message_type));
                continue;
            }

            switch(message_type){
                case Codes::Message_type::Fluorometer_OJIP_capture_request: {
                    Logger::Notice("Fluorometer OJIP capture start");
                    App_messages::Fluorometer::OJIP_capture_request ojip_request;
                    ojip_request.Interpret_data(message.data);

                    if (not ojip_request.Interpret_data(message.data)) {
                        Logger::Error("Fluorometer OJIP Capture request interpretation failed");
                        break;
                    }

                    if (not fluorometer->ojip_capture_finished) {
                        Logger::Warning("Fluorometer OJIP Capture in progress");
                        break;
                    }

                    Logger::Notice("Starting capture with gain: {:2.0f}, intensity: {:04.2f}, length: {:3.1f}s, samples: {:d}",
                                                (float)Fluorometer_config::gain_values.at(ojip_request.detector_gain),
                                                (float)ojip_request.emitor_intensity,
                                                (float)(ojip_request.length_ms/1000.0f),
                                                (int)ojip_request.samples
                    );

                    fluorometer->OJIP_data.measurement_id = ojip_request.measurement_id;
                    fluorometer->OJIP_data.emitor_intensity = ojip_request.emitor_intensity;

                    fluorometer->Capture_OJIP(ojip_request.detector_gain, ojip_request.emitor_intensity, (ojip_request.length_ms/1000.0f), ojip_request.samples, ojip_request.sample_timing);

                    fluorometer->OJIP_data.detector_gain = fluorometer->Gain();
                } break;

                case Codes::Message_type::Fluorometer_OJIP_retrieve_request: {
                    Logger::Notice("Fluorometer OJIP export started");
                    if(!fluorometer->ojip_capture_finished) {
                        Logger::Warning("Fluorometer OJIP capture not finished");
                        break;
                    }

                    if(fluorometer->OJIP_data.intensity.size() == 0) {
                        Logger::Warning("Fluorometer OJIP data empty");
                        break;
                    }

                    fluorometer->Export_data(&fluorometer->OJIP_data);
                } break;

                case Codes::Message_type::Fluorometer_calibration_request: {
                    Logger::Notice("Fluorometer calibration request");
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
