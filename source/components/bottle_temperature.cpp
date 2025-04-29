#include "bottle_temperature.hpp"

Bottle_temperature::Bottle_temperature(Thermopile * top_sensor, Thermopile * bottom_sensor):
    Component(Codes::Component::Bottle_temperature),
    Message_receiver(Codes::Component::Bottle_temperature),
    top_sensor(top_sensor),
    bottom_sensor(bottom_sensor)
{
}

float Bottle_temperature::Temperature(){
    return (Top_temperature() + Bottom_temperature()) / 2;
}

float Bottle_temperature::Top_temperature(){
    return top_sensor->Temperature();
}

float Bottle_temperature::Bottom_temperature(){
    return bottom_sensor->Temperature();
}

float Bottle_temperature::Bottom_sensor_temperature(){
    return bottom_sensor->Ambient();
}

float Bottle_temperature::Top_sensor_temperature(){
    return top_sensor->Ambient();
}

bool Bottle_temperature::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Bottle_temperature::Receive(Application_message message){
    // Initialize temperature filters during first request
    if(not temperature_initialized){
        Logger::Debug("Bottle temperature initialization");
        top_sensor->Init_filters();
        bottom_sensor->Init_filters();
        temperature_initialized = true;
    }

    switch (message.Message_type()) {
        case Codes::Message_type::Bottle_temperature_request: {
            App_messages::Bottle_temperature::Temperature_response response(Temperature());
            Logger::Debug("Bottle temperature: {:05.2f}°C", response.temperature);
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Bottle_top_measured_temperature_request: {
            App_messages::Bottle_temperature::Top_measured_temperature_response response(Top_temperature());
            Logger::Debug("Top measured temperature: {:05.2f}°C", response.temperature);
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Bottle_bottom_measured_temperature_request: {
            App_messages::Bottle_temperature::Bottom_measured_temperature_response response(Bottom_temperature());
            Logger::Debug("Bottom measured temperature: {:05.2f}°C", response.temperature);
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Bottle_top_sensor_temperature_request: {
            App_messages::Bottle_temperature::Top_sensor_temperature_response response(Top_sensor_temperature());
            Logger::Debug("Top sensor temperature: {:05.2f}°C", response.temperature);
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Bottle_bottom_sensor_temperature_request: {
            App_messages::Bottle_temperature::Bottom_sensor_temperature_response response(Bottom_sensor_temperature());
            Logger::Debug("Bottom sensor temperature: {:05.2f}°C", response.temperature);
            Send_CAN_message(response);
            return true;
        }

        default:
            return false;
    }
}
