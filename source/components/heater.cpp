#include "heater.hpp"

Heater::Heater(uint gpio_in1, uint gpio_in2, float pwm_frequency):
    Component(Codes::Component::Bottle_heater),
    Message_receiver(Codes::Component::Bottle_heater),
    control_bridge(new DC_HBridge_PIO(gpio_in1, gpio_in2, PIO_machine(pio0,3), pwm_frequency)),
    heater_sensor(new Thermistor(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_3, 3.30f), 3950, 100000, 25, 30000)),
    heater_fan(new GPIO(11, GPIO::Direction::Out))
{
    control_bridge->Coast();
    heater_fan->Set(false);
}

float Heater::Intensity(float requested_intensity){
    // Reduce intensity of higher then maximal allowed
    this->intensity = std::clamp(requested_intensity, -intensity_limit, intensity_limit);

    // Start heater fan if heater is in use
    if(intensity != 0){
        heater_fan->Set(true);
    } else {
        heater_fan->Set(false);
    }

    // Set intensity of heater
    control_bridge->Speed(intensity);
    return intensity;
}

float Heater::Intensity() const {
    return intensity;
}

float Heater::Temperature(){
     return heater_sensor->Temperature();
}

bool Heater::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Heater::Receive(Application_message message){
    switch (message.Message_type()){
        case Codes::Message_type::Heater_set_intensity:{
            App_messages::Heater::Set_intensity set_intensity;

            if (!set_intensity.Interpret_data(message.data)){
                Logger::Print("Heater_set_intensity interpretation failed");
                return false;
            }
            Logger::Print(emio::format("Heater intensity set to: {:03.1f}", set_intensity.intensity));
            Intensity(set_intensity.intensity);
            return true;
        }

        case Codes::Message_type::Heater_get_intensity_request:{
            App_messages::Heater::Get_intensity_response intensity_response(Intensity());
            Send_CAN_message(intensity_response);
            return true;
        }

        case Codes::Message_type::Heater_set_target_temperature:
            return false;

        case Codes::Message_type::Heater_get_target_temperature_request:
            return false;

        case Codes::Message_type::Heater_get_plate_temperature_request:{
            float temp = Temperature();
            Logger::Print(emio::format("Heater plate temperature: {:05.2f}˚C", temp));
            App_messages::Heater::Get_plate_temperature_response plate_temperature(temp);
            Send_CAN_message(plate_temperature);
            return true;
        }

        default:
            return false;
    }
}
