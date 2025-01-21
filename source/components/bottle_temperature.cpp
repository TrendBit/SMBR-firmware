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
    switch (message.Message_type()) {
        case Codes::Message_type::Bottle_temperature_request: {
            App_messages::Bottle_temperature::Temperature_response response(Temperature());
            Logger::Print(emio::format("Bottle temperature: {:05.2f}°C", response.temperature));
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Bottle_top_measured_temperature_request: {
            App_messages::Bottle_temperature::Top_measured_temperature_response response(Top_temperature());
            Logger::Print(emio::format("Top measured temperature: {:05.2f}°C", response.temperature));
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Bottle_bottom_measured_temperature_request: {
            App_messages::Bottle_temperature::Bottom_measured_temperature_response response(Bottom_temperature());
            Logger::Print(emio::format("Bottom measured temperature: {:05.2f}°C", response.temperature));
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Bottle_top_sensor_temperature_request: {
            App_messages::Bottle_temperature::Top_sensor_temperature_response response(Top_sensor_temperature());
            Logger::Print(emio::format("Top sensor temperature: {:05.2f}°C", response.temperature));
            Send_CAN_message(response);
            return true;
        }

        case Codes::Message_type::Bottle_bottom_sensor_temperature_request: {
            App_messages::Bottle_temperature::Bottom_sensor_temperature_response response(Bottom_sensor_temperature());
            Logger::Print(emio::format("Bottom sensor temperature: {:05.2f}°C", response.temperature));
            Send_CAN_message(response);
            return true;
        }

        default:
            return false;
    }
}
