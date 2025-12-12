#include "pumps.hpp"

Pump::Pump(uint8_t gpio_in1, uint8_t gpio_in2, uint8_t indication_pin, float max_flowrate, float min_speed, float pwm_frequency) :
    DC_HBridge(gpio_in1, gpio_in2, pwm_frequency, DC_HBridge::Stop_mode::Brake)
{
    indication = new PWM_channel(indication_pin, 100.0f, 1.0f, true);
}

void Pump::Speed(float speed){
    DC_HBridge::Speed(speed);
    Indicate(speed);
}

void Pump::Indicate(float intensity){
    intensity = std::clamp(intensity, 0.0f, 1.0f);

    // Apply gamma correction for human perception (gamma = 2.2)
    float perceived_intensity = std::pow(intensity, 2.2f);

    indication->Duty_cycle(1.0f-perceived_intensity);
}

Pump_controller::Pump_controller(etl::vector<Pump *,8> pumps):
    Component(Codes::Component::Pumps),
    Message_receiver(Codes::Component::Pumps),
    pumps(pumps)
    {
    Logger::Debug("Pumps component initialized");

    // Blink with pump status LEDs after startup
    new rtos::Execute_until([pumps]() -> bool {
        static uint count = 0;
        const uint max_count = 20;

        for(auto pump : pumps){
            pump->Indicate(static_cast<float>(count%2));
        }

        count++;

        if(count >= max_count){
            for(auto pump : pumps){
                pump->Indicate(0.0f);
            }

            return true;
        } else {
            return false;
        }
    }, 50, true, true);
}

bool Pump_controller::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Pump_controller::Receive(Application_message message){
    switch (message.Message_type()) {
        case Codes::Message_type::Pumps_pump_count_request: {
            App_messages::Pumps::Pump_count_response pump_count_response(Pump_count());

            Logger::Debug("Pumps count requested, response: {}", Pump_count());
            Send_CAN_message(pump_count_response);
            return true;
        }

        case Codes::Message_type::Pumps_set_speed: {
            App_messages::Pumps::Set_speed set_speed;

            if (!set_speed.Interpret_data(message.data)) {
                Logger::Error("Pumps_set_speed interpretation failed");
                return false;
            }

            if (set_speed.pump_index == 0 || set_speed.pump_index > Pump_count()) {
                Logger::Error("Pumps_set_speed invalid pump index: {}", set_speed.pump_index);
                return false;
            }

            Logger::Debug("Pump {} speed set to: {:03.2f}", set_speed.pump_index, set_speed.speed);
            pumps[set_speed.pump_index - 1]->Speed(set_speed.speed);
            return true;
        }

        default:
            Logger::Warning("Pumps component received unknown message type: {}", static_cast<uint16_t>(message.Message_type()));
            return false;
    }
}
