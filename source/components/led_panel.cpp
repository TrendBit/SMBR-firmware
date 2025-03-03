#include "led_panel.hpp"

LED_panel::LED_panel(std::vector<LED_intensity *> &channels, Thermistor * temp_sensor, float power_budget_w):
    Component(Codes::Component::LED_panel),
    Message_receiver(Codes::Component::LED_panel),
    channels(channels),
    temp_sensor(temp_sensor),
    power_budget_w(power_budget_w)
{
}

bool LED_panel::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool LED_panel::Receive(Application_message message){
    switch (message.Message_type()){
        case Codes::Message_type::LED_set_intensity:{
            App_messages::LED_panel::Set_intensity led_set_intensity;
            if (!led_set_intensity.Interpret_data(message.data)){
                Logger::Print("LED_set_intensity interpretation failed", Logger::Level::Error);
                return false;
            }
            Set_intensity(led_set_intensity.channel, led_set_intensity.intensity);
            return true;
        }

        case Codes::Message_type::LED_get_intensity_request:{
            App_messages::LED_panel::Get_intensity_request led_get_intensity;
            if (!led_get_intensity.Interpret_data(message.data)){
                Logger::Print("LED_get_intensity_request interpretation failed", Logger::Level::Error);
                return false;
            }
            Get_intensity(led_get_intensity.channel);
            return true;
        }

        case Codes::Message_type::LED_get_temperature_request:{
            App_messages::LED_panel::Temperature_request Ttemperature;
            Get_temperature();
            return true;
        }

        default:
            return false;
    }
}

bool LED_panel::Set_intensity(uint8_t channel, float intensity){
    if (channel >= channels.size()){
        Logger::Print("LED channel out of range", Logger::Level::Error);
        return false;
    }
    Logger::Print(emio::format("Set LED intensity Channel: {:d}, Intensity: {:04.2f}", channel, intensity), Logger::Level::Debug);
    channels[channel]->Intensity(intensity);
    return true;
}

bool LED_panel::Get_intensity(uint8_t channel){
    if (channel >= channels.size()){
        Logger::Print("LED channel out of range", Logger::Level::Error);
        return false;
    }
    float intensity = channels[channel]->Intensity();
    Logger::Print(emio::format("Get LED intensity Channel: {:d}, Intensity: {:04.2f}", channel, intensity), Logger::Level::Debug);
    App_messages::LED_panel::Get_intensity_response response(channel, intensity);
    Send_CAN_message(response);
    return true;
}

bool LED_panel::Get_temperature(){
    if (temp_sensor == nullptr){
        Logger::Print("Temperature sensor not available", Logger::Level::Error);
        return false;
    }
    float temp = temp_sensor->Temperature();
    Logger::Print(emio::format("LED panel temperature: {:05.2f}˚C", temp), Logger::Level::Debug);
    App_messages::LED_panel::Temperature_response response(temp);
    Send_CAN_message(response);
    return true;
}


float LED_panel::Temperature() const{
    if (temp_sensor == nullptr){
        Logger::Print("Temperature sensor not available");
        return std::numeric_limits<double>::quiet_NaN();
    }
    return temp_sensor->Temperature();
}
