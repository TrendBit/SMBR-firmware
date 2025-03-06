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

    // Register messages to received for regulator
    Message_router::Register_bypass(Codes::Message_type::Bottle_temperature_response, Codes::Component::Bottle_heater);

    // Prepare regulation loop, do not start it until target temperature is set
    auto regulation_lambda = [this](){ this->Regulation_loop(); };
    regulation_loop = new rtos::Repeated_execution(regulation_lambda, 5000, false);
}

void Heater::Regulation_loop() {
    Logger::Print("Regulating heater intensity", Logger::Level::Trace);

    if (!target_temperature.has_value()) {
        Logger::Print("No target temperature set, regulation disabled", Logger::Level::Notice);
        regulation_loop->Disable();
        integral_error = 0.0f;  // Reset integral when regulation is disabled
        return;
    }

    if(bottle_temperature.has_value()) {
        float current_intensity = Intensity();

        float temp_diff;

        // Limit intensity based on max temp of heater plate
        float plate_current_temperature = Temperature();
        if (plate_current_temperature > plate_max_temperature) {
            Logger::Print("Heater plate temperature exceeded, limiting power output", Logger::Level::Warning);
            temp_diff = plate_max_temperature - plate_current_temperature;
        } else {
            temp_diff = target_temperature.value() - bottle_temperature.value();
        }

        // Update integral term (with anti-windup)
        if (std::abs(current_intensity) < intensity_limit) {
            integral_error += temp_diff;
            // Apply anti-windup by limiting the integral error
            integral_error = std::clamp(integral_error, -integral_limit, integral_limit);
        }

        // Calculate P and I components
        float p_component = temp_diff * p_gain;
        float i_component = integral_error * i_gain;

        // Calculate desired intensity from P+I
        float desired_intensity = std::clamp(p_component + i_component, -1.0f, 1.0f);

        // Limit the size of the step
        float change = desired_intensity - current_intensity;
        float step_size = std::clamp(change, -regulation_step, regulation_step);

        // Calculate new intensity
        float new_intensity = std::clamp(
            current_intensity + step_size,
            -1.0f,  // Allow negative for cooling
            1.0f    // Positive for heating
        );

        // Apply new intensity
        Intensity(new_intensity);

        // Enhanced logging with P and I component details
        Logger::Print(emio::format("Current temp: {:03.1f}, target temp: {:03.1f}, diff: {:+03.1f}",
                     bottle_temperature.value(), target_temperature.value(), temp_diff), Logger::Level::Notice);
        Logger::Print(emio::format("PI-control: P={:+04.2f}, I={:+04.2f}, desired={:+04.2f}, step={:+04.2f}, new={:04.2f}",
                     p_component, i_component, desired_intensity, step_size, new_intensity), Logger::Level::Notice);

        bottle_temperature = std::nullopt;
    } else {
        Logger::Print("No bottle temperature received, regulation disabled", Logger::Level::Warning);
        Intensity(0);
        return;
    }

    Request_bottle_temperature();
}

float Heater::Compensate_intensity(float requested_intensity) {
    // Find interval in power curve
    for (size_t i = 1; i < power_curve.size(); i++) {
        if (power_curve[i].out >= requested_intensity) {
            // Linear interpolation
            float t = (requested_intensity - power_curve[i-1].out) /
                     (power_curve[i].out - power_curve[i-1].out);
            return power_curve[i-1].set +
                   t * (power_curve[i].set - power_curve[i-1].set);
        }
    }
    return requested_intensity;
}

float Heater::Intensity(float requested_intensity){
    // Store the original requested intensity (to return at the end)
    intensity = requested_intensity;

    float scaled_intensity = requested_intensity * intensity_limit;

    // Linearize power curve of heater to compensate nonlinearity of heater intensity
    float compensated_intensity = Compensate_intensity(std::abs(intensity));

    // Apply sign for heating/cooling
    compensated_intensity = std::copysign(compensated_intensity, intensity);

    Logger::Print(emio::format("Heater requested: {:03.1f}, scaled: {:03.1f}, compensated: {:03.1f} ", requested_intensity, scaled_intensity, compensated_intensity), Logger::Level::Notice);

    // Start heater fan if heater is in use
    if(requested_intensity != 0){
        heater_fan->Set(true);
    } else {
        heater_fan->Set(false);
    }

    // Set intensity of heater
    control_bridge->Speed(compensated_intensity);
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
                Logger::Print("Heater_set_intensity interpretation failed", Logger::Level::Error);
                return false;
            }
            Logger::Print(emio::format("Heater intensity set to: {:03.1f}", set_intensity.intensity), Logger::Level::Debug);
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
                Logger::Print("Heater_set_target_temperature interpretation failed", Logger::Level::Error);
                return false;
            }
            Logger::Print(emio::format("Heater target temperature set to: {:05.2f}˚C", set_target_temperature.temperature), Logger::Level::Debug);
            target_temperature = set_target_temperature.temperature;
            Request_bottle_temperature();
            regulation_loop->Enable();
            return true;
        }

        case Codes::Message_type::Heater_get_target_temperature_request:{
            float target_temp = target_temperature.value_or(std::numeric_limits<float>::quiet_NaN());
            Logger::Print(emio::format("Heater target is temperature: {:05.2f}˚C", target_temp), Logger::Level::Debug);
            App_messages::Heater::Get_target_temperature_response target_temperature_response(target_temp);
            Send_CAN_message(target_temperature_response);
            return true;
        }

        case Codes::Message_type::Heater_get_plate_temperature_request:{
            float temp = Temperature();
            Logger::Print(emio::format("Heater plate temperature: {:05.2f}˚C", temp), Logger::Level::Debug);
            App_messages::Heater::Get_plate_temperature_response plate_temperature(temp);
            Send_CAN_message(plate_temperature);
            return true;
        }

        case Codes::Message_type::Heater_turn_off:{
            Logger::Print("Heater turned off", Logger::Level::Debug);
            Turn_off();
            return true;
        }

        case Codes::Message_type::Bottle_temperature_response:{
            App_messages::Bottle_temperature::Temperature_response temperature_response;
            if (!temperature_response.Interpret_data(message.data)){
                Logger::Print("Bottle_temperature_response interpretation failed", Logger::Level::Error);
                return false;
            }
            bottle_temperature = temperature_response.temperature;
            Logger::Print(emio::format("Bottle temperature: {:05.2f}˚C", bottle_temperature.value_or(0.0)), Logger::Level::Debug);
            return true;
        }

        default:
            return false;
    }
}
