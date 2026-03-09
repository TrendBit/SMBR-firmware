#include "pumps.hpp"

Pump::Pump(uint8_t gpio_in1, uint8_t gpio_in2, uint8_t indication_pin, std::unique_ptr<Current_sensor> current_sensor, float max_flowrate, float min_speed, float pwm_frequency) :
    DC_HBridge(gpio_in1, gpio_in2, pwm_frequency, DC_HBridge::Stop_mode::Brake),
    indication(std::make_unique<PWM_channel>(indication_pin, 100.0f, 1.0f, true)),
    current_sensor(std::move(current_sensor)),
    max_flowrate(max_flowrate)
{
    auto stopper_lambda = [this]() {
        Stop();
    };
    pump_stopper = new rtos::Delayed_execution(stopper_lambda);
}

void Pump::Speed(float speed){
    float in = std::clamp(speed, -1.0f, 1.0f);
    if (std::abs(in) < 1e-4f) {
        Stop();
        return;
    }

    float magnitude = motor_pump_speed_curve.To_speed(std::abs(in));
    float signed_speed = (in >= 0.0f) ? magnitude : -magnitude;
    float clamped = std::clamp(signed_speed, -1.0f, 1.0f);
    DC_HBridge::Speed(clamped);
    Indicate(std::abs(in));
}

float Pump::Speed() {
    float motor_speed = DC_HBridge::Speed();
    float pump_speed = motor_pump_speed_curve.To_rate(std::abs(motor_speed));
    return (motor_speed >= 0.0f) ? pump_speed : -pump_speed;
}

void Pump::Flowrate(float flowrate){
    float pump_speed = flowrate / Maximal_flowrate();
    Speed(pump_speed);
}

float Pump::Flowrate() {
    float speed = Speed();
    float direction = speed >= 0.0f ? 1.0f : -1.0f;
    float flowrate = (motor_pump_speed_curve.To_rate(std::abs(speed))) * Maximal_flowrate();
    flowrate *= direction;
    return flowrate;
}

float Pump::Set_Maximal_flowrate(float flowrate){
    max_flowrate = flowrate;
    return flowrate;
}

float Pump::Maximal_flowrate() const {
    return max_flowrate;
}

float Pump::Minimal_flowrate() const {
    return motor_pump_speed_curve.Min_rate() * max_flowrate;
}

void Pump::Stop(){
    pump_stopper->Abort();
    Indicate(0.0f);
    DC_HBridge::Stop();
}

float Pump::Move(float volume_ml, float flowrate){
    float effective_flowrate = std::clamp(flowrate, Minimal_flowrate(), Maximal_flowrate());

    float pump_time_sec = (std::abs(volume_ml) / effective_flowrate) * 60.0f;

    Logger::Debug("Pump move volume: {:03.1f}, effective_flowrate: {:03.1f}, time: {:03.1f}s",
        volume_ml, effective_flowrate, pump_time_sec);

    if (volume_ml > 0) {
        Flowrate(effective_flowrate);
    } else {
        Flowrate(-effective_flowrate);
    }

    pump_stopper->Execute(pump_time_sec * 1000.0f);

    return pump_time_sec;
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

Pump_controller::Pump_controller(etl::vector<Pump *,8> pumps, EEPROM_storage * const memory):
    Component(Codes::Component::Pumps),
    Message_receiver(Codes::Component::Pumps),
    pumps(pumps),
    memory(memory)
    {
    Logger::Debug("Pumps component initialized");
    Load_max_flowrates();

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

void Pump_controller::Load_max_flowrates(){
    for (uint8_t index = 0; index < Pump_count(); index++) {
        std::optional<float> max_flowrate_mem_o = memory->Read_Pump_max_flowrate(index);
        if (!max_flowrate_mem_o.has_value()) {
            pumps[index]->Set_Maximal_flowrate(Pump::fallback_max_flowrate);
            Logger::Notice("Pump {} max flowrate fallback: {:f}", index + 1, Pump::fallback_max_flowrate);
            continue;
        }
        float max_flowrate_mem = max_flowrate_mem_o.value();
        if ((max_flowrate_mem >= pumps[index]->Minimal_flowrate()) && (max_flowrate_mem <= 1000.0f)) {
            // Loaded float is in valid range
            pumps[index]->Set_Maximal_flowrate(max_flowrate_mem);
            Logger::Notice("Pump {} max flowrate: {:f}", index + 1, max_flowrate_mem);
        } else {
            pumps[index]->Set_Maximal_flowrate(Pump::fallback_max_flowrate);
            Logger::Notice("Pump {} max flowrate fallback: {:f}", index + 1, Pump::fallback_max_flowrate);
        }
    }
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

        case Codes::Message_type::Pumps_set_max_flowrate: {
            App_messages::Pumps::Set_max_flowrate set_max_flowrate;
            if (!set_max_flowrate.Interpret_data(message.data)) {
                Logger::Error("Pumps_set_max_flowrate interpretation failed");
                return false;
            }

            if (not Valid_pump_index(set_max_flowrate.pump_index)) {
                Logger::Error("Pumps_set_max_flowrate invalid pump index: {}", set_max_flowrate.pump_index);
                return false;
            }

            Logger::Debug("Pump {} maximal flowrate set to: {:03.2f}", set_max_flowrate.pump_index, set_max_flowrate.max_flow_rate);
            pumps[set_max_flowrate.pump_index - 1]->Set_Maximal_flowrate(set_max_flowrate.max_flow_rate);

            std::optional<float> written = memory->Write_Pump_max_flowrate(set_max_flowrate.pump_index - 1, set_max_flowrate.max_flow_rate);
            if (!written.has_value()) {
                Logger::Error("Failed to write pump {} max flowrate to memory", set_max_flowrate.pump_index);
                return false;
            }
            return true;
        }

        case Codes::Message_type::Pumps_info_request: {
            App_messages::Pumps::Info_request info_request;
            if (!info_request.Interpret_data(message.data)) {
                Logger::Error("Pumps_info_request interpretation failed");
                return false;
            }

            if (not Valid_pump_index(info_request.pump_index)) {
                Logger::Error("Pumps_info_request invalid pump index: {}", info_request.pump_index);
                return false;
            }

            Logger::Debug("Pump {} info requested", info_request.pump_index);
            float min_flowrate = pumps[info_request.pump_index - 1]->Minimal_flowrate();
            float max_flowrate = pumps[info_request.pump_index - 1]->Maximal_flowrate();
            App_messages::Pumps::Info_response info_response(info_request.pump_index, min_flowrate, max_flowrate);
            Send_CAN_message(info_response);
            return true;
        }

        case Codes::Message_type::Pumps_move: {
            App_messages::Pumps::Move move_message;
            if (!move_message.Interpret_data(message.data)) {
                Logger::Error("Pumps_move interpretation failed");
                return false;
            }

            if (not Valid_pump_index(move_message.pump_index)) {
                Logger::Error("Pumps_move invalid pump index: {}", move_message.pump_index);
                return false;
            }

            Logger::Debug("Pump {} move volume: {:03.1f}, flowrate: {:03.1f}", move_message.pump_index, move_message.volume, move_message.flowrate);
            pumps[move_message.pump_index - 1]->Move(move_message.volume, move_message.flowrate);
            return true;
        }

        // TODO Move


        default:
            Logger::Warning("Pumps component received unknown message type: {}", static_cast<uint16_t>(message.Message_type()));
            return false;
    }
}
