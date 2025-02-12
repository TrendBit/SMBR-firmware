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
    Message_router::Register_bypass(Codes::Message_type::Bottle_temperature_response, Codes::Component::Bottle_heater);

    auto regulation_lambda = [this](){
        Logger::Print("Regulating heater intensity");

        if (!target_temperature.has_value()){
            Logger::Print("No target temperature set, regulation disabled");
            regulation_loop->Abort();
            return;
        } else {
            regulation_loop->Execute(5000);
        }

        if(bottle_temperature.has_value()){
            float current_intensity = Intensity();
            float new_intensity;
            if (bottle_temperature.value() < target_temperature.value()){
                new_intensity = std::clamp(current_intensity + 0.1f, 0.0f, intensity_limit);
                Intensity(new_intensity);
            } else {
                new_intensity = std::clamp(current_intensity - 0.1f, 0.0f, intensity_limit);
                Intensity(new_intensity);
            }
            Logger::Print(emio::format("Current temp: {:03.1f}, target temp: {:03.1f}", bottle_temperature.value(), target_temperature.value()));
            Logger::Print(emio::format("Heater intensity set to: {:03.1f}", new_intensity));
            bottle_temperature = std::nullopt;
        } else {
            Logger::Print("No bottle temperature received, regulation disabled");
            Intensity(0);
            regulation_loop->Abort();
            return;
        }

        Request_bottle_temperature();
    };

    regulation_loop = new rtos::Delayed_execution(regulation_lambda, 20000, false);
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

void Heater::Turn_off(){
    target_temperature.reset();
    Intensity(0);
}

bool Heater::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Heater::Request_bottle_temperature(){
    Application_message temp_request(Codes::Module::Sensor_module, Codes::Instance::Exclusive, Codes::Message_type::Bottle_temperature_request);
    return Send_CAN_message(temp_request);
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

        case Codes::Message_type::Heater_set_target_temperature:{
            App_messages::Heater::Set_target_temperature set_target_temperature;
            if (!set_target_temperature.Interpret_data(message.data)){
                Logger::Print("Heater_set_target_temperature interpretation failed");
                return false;
            }
            Logger::Print(emio::format("Heater target temperature set to: {:05.2f}˚C", set_target_temperature.temperature));
            target_temperature = set_target_temperature.temperature;
            Request_bottle_temperature();
            regulation_loop->Execute(2000);
            return true;
        }

        case Codes::Message_type::Heater_get_target_temperature_request:{
            float target_temp = target_temperature.value_or(std::numeric_limits<float>::quiet_NaN());
            Logger::Print(emio::format("Heater target is temperature: {:05.2f}˚C", target_temp));
            App_messages::Heater::Get_target_temperature_response target_temperature_response(target_temp);
            Send_CAN_message(target_temperature_response);
            return true;
        }

        case Codes::Message_type::Heater_get_plate_temperature_request:{
            float temp = Temperature();
            Logger::Print(emio::format("Heater plate temperature: {:05.2f}˚C", temp));
            App_messages::Heater::Get_plate_temperature_response plate_temperature(temp);
            Send_CAN_message(plate_temperature);
            return true;
        }

        case Codes::Message_type::Heater_turn_off:{
            Logger::Print("Heater turned off");
            Turn_off();
            return true;
        }

        case Codes::Message_type::Bottle_temperature_response:{
            App_messages::Bottle_temperature::Temperature_response temperature_response;
            if (!temperature_response.Interpret_data(message.data)){
                Logger::Print("Bottle_temperature_response interpretation failed");
                return false;
            }
            bottle_temperature = temperature_response.temperature;
            Logger::Print(emio::format("Bottle temperature: {:05.2f}˚C", bottle_temperature.value_or(0.0)));
            return true;
        }

        default:
            return false;
    }
}
