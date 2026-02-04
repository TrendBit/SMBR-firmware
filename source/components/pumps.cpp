#include "pumps.hpp"

Pump::Pump(uint8_t gpio_in1, uint8_t gpio_in2, uint8_t indication_pin, std::unique_ptr<Current_sensor> current_sensor, float max_flowrate, float min_speed, float pwm_frequency) :
    DC_HBridge(gpio_in1, gpio_in2, pwm_frequency, DC_HBridge::Stop_mode::Brake),
    indication(std::make_unique<PWM_channel>(indication_pin, 100.0f, 1.0f, true)),
    current_sensor(std::move(current_sensor)),
    max_flowrate(max_flowrate)
{

}

void Pump::Speed(float speed){
    DC_HBridge::Speed(speed);
    Indicate(speed);
}

float Pump::Speed() const {
    return DC_HBridge::Speed();
}

void Pump::Flowrate(float flowrate){
    DC_HBridge::Speed((flowrate / max_flowrate));
}

float Pump::Flowrate() const {
    return DC_HBridge::Speed() * max_flowrate;
}

void Pump::Stop(){
    DC_HBridge::Stop();
}

void Pump::Indicate(float intensity){
    intensity = std::clamp(intensity, 0.0f, 1.0f);

    // Apply gamma correction for human perception (gamma = 2.2)
    float perceived_intensity = std::pow(intensity, 2.2f);

    indication->Duty_cycle(1.0f-perceived_intensity);
}

float Pump::Current(){
    return current_sensor->Current();
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

    new rtos::Repeated_execution([pumps]() {
        Logger::Notice("----------------------");
        for(uint i = 0; i < 1; i++){
            float current = pumps[i]->Current();
            Logger::Notice("Pump {} current: {:0.2f} A", i+1, current);
        }
        Logger::Notice("----------------------");
    }, 1000, true);
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

        case Codes::Message_type::Pumps_get_speed_request: {
            App_messages::Pumps::Get_speed_request get_speed_request;
            if (!get_speed_request.Interpret_data(message.data)) {
                Logger::Error("Pumps_get_speed_request interpretation failed");
                return false;
            }

            if (not Valid_pump_index(get_speed_request.pump_index)) {
                Logger::Error("Pumps_get_speed_request invalid pump index: {}", get_speed_request.pump_index);
                return false;
            }

            Logger::Debug("Pump {} speed requested", get_speed_request.pump_index);
            float speed = pumps[get_speed_request.pump_index - 1]->Speed();
            App_messages::Pumps::Get_speed_response get_speed_response(get_speed_request.pump_index, speed);
            Send_CAN_message(get_speed_response);
            return true;
        }

        case Codes::Message_type::Pumps_set_flowrate: {
            App_messages::Pumps::Set_flowrate set_flowrate;
            if (!set_flowrate.Interpret_data(message.data)) {
                Logger::Error("Pumps_set_flowrate interpretation failed");
                return false;
            }

            if (not Valid_pump_index(set_flowrate.pump_index)) {
                Logger::Error("Pumps_set_flowrate invalid pump index: {}", set_flowrate.pump_index);
                return false;
            }

            Logger::Debug("Pump {} flowrate set to: {:03.2f}", set_flowrate.pump_index, set_flowrate.flowrate);
            pumps[set_flowrate.pump_index - 1]->Flowrate(set_flowrate.flowrate);
            return true;
        }

        case Codes::Message_type::Pumps_get_flowrate_request: {
            App_messages::Pumps::Get_flowrate_request get_flowrate_request;
            if (!get_flowrate_request.Interpret_data(message.data)) {
                Logger::Error("Pumps_get_flowrate_request interpretation failed");
                return false;
            }

            if (not Valid_pump_index(get_flowrate_request.pump_index)) {
                Logger::Error("Pumps_get_flowrate_request invalid pump index: {}", get_flowrate_request.pump_index);
                return false;
            }

            Logger::Debug("Pump {} flowrate requested", get_flowrate_request.pump_index);
            float flowrate = pumps[get_flowrate_request.pump_index - 1]->Flowrate();
            App_messages::Pumps::Get_flowrate_response get_flowrate_response(get_flowrate_request.pump_index, flowrate);
            Send_CAN_message(get_flowrate_response);
            return true;
        }

        case Codes::Message_type::Pumps_stop: {
            App_messages::Pumps::Stop stop;
            if (!stop.Interpret_data(message.data)) {
                Logger::Error("Pumps_stop interpretation failed");
                return false;
            }

            if (not Valid_pump_index(stop.pump_index)) {
                Logger::Error("Pumps_stop invalid pump index: {}", stop.pump_index);
                return false;
            }

            Logger::Debug("Pump {} stopped", stop.pump_index);
            pumps[stop.pump_index - 1]->Stop();
            return true;
        }

        case Codes::Message_type::Pumps_stop_all: {
            Logger::Debug("Pumps stop all");
            for (auto pump : pumps) {
                pump->Stop();
            }
            return true;
        }

        // TODO Info, Move, Max_flow(Calibrate)


        default:
            Logger::Warning("Pumps component received unknown message type: {}", static_cast<uint16_t>(message.Message_type()));
            return false;
    }
}
