#include "led_illumination.hpp"

LED_illumination::LED_illumination(std::vector<LED_intensity *> &channels, Thermistor * temp_sensor, float power_budget_w):
    Message_receiver(Codes::Component::LED_illumination),
    channels(channels),
    temp_sensor(temp_sensor),
    power_budget_w(power_budget_w)
{
}

bool LED_illumination::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool LED_illumination::Receive(Application_message message){
    switch (message.Message_type()){
        case Codes::Message_type::LED_set_intensity:{
            App_messages::LED_set_intensity led_set_intensity;
            if (!led_set_intensity.Interpret_app_message(message)){
                Logger::Print("LED_set_intensity interpretation failed");
                return false;
            }
            Set_intensity(led_set_intensity.channel, led_set_intensity.intensity);
            return true;
        }

        default:
            return false;
    }
}

bool LED_illumination::Set_intensity(uint8_t channel, float intensity){
    if (channel >= channels.size()){
        Logger::Print("LED channel out of range");
        return false;
    }
    Logger::Print(emio::format("Set LED intensity Channel: {:d}, Intensity: {:04.2f}", channel, intensity));
    channels[channel]->Intensity(intensity);
    return true;
}
