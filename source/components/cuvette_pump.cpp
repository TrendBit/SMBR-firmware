#include "cuvette_pump.hpp"

Cuvette_pump::Cuvette_pump(uint gpio_in1, uint gpio_in2, float cuvette_system_volume, EEPROM_storage * const memory, float min_speed, float pwm_frequency) :
    Component(Codes::Component::Cuvette_pump),
    Message_receiver(Codes::Component::Cuvette_pump),
    DC_HBridge(gpio_in1, gpio_in2, pwm_frequency, DC_HBridge::Stop_mode::Brake),
    cuvette_system_volume(cuvette_system_volume),
    memory(memory)
{
    auto stopper_lamda = [this](){
          Stop();
    };
    pump_stopper = new rtos::Delayed_execution(stopper_lamda);
    Load_max_flowrate();
}

void Cuvette_pump::Load_max_flowrate(){
    std::optional<float> max_flowrate_mem_o = memory->Read_Cuvette_pump_max_flowrate();
    if (!max_flowrate_mem_o.has_value()) {
        max_flowrate = fallback_max_flowrate;
        return;
    }
    float max_flowrate_mem = max_flowrate_mem_o.value();
    if ((max_flowrate_mem >= Minimal_flowrate()) && (max_flowrate_mem <= 1000.0f)) {
        // Loaded float is in valid range
        max_flowrate = max_flowrate_mem;
    } else {
        max_flowrate = fallback_max_flowrate;
    }
    Logger::Notice("Cuvette_pump max flowrate: {:f}", max_flowrate);
}

bool Cuvette_pump::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Cuvette_pump::Receive(Application_message message){
    switch (message.Message_type()) {
        case Codes::Message_type::Cuvette_pump_set_speed: {
            App_messages::Cuvette_pump::Set_speed set_speed;

            if (!set_speed.Interpret_data(message.data)) {
                Logger::Error("Cuvette_pump_set_speed interpretation failed");
                return false;
            }

            Logger::Debug("Cuvette pump speed set to: {:03.1f}", set_speed.speed);
            Speed(set_speed.speed);
            return true;
        }

        case Codes::Message_type::Cuvette_pump_get_speed_request: {
            App_messages::Cuvette_pump::Get_speed_response speed_response(Speed());
            Logger::Debug("Cuvette pump speed requested, response: {:03.1f}", speed_response.speed);
            Send_CAN_message(speed_response);
            return true;
        }

        case Codes::Message_type::Cuvette_pump_set_flowrate: {
            App_messages::Cuvette_pump::Set_flowrate set_flowrate;

            if (!set_flowrate.Interpret_data(message.data)) {
                Logger::Error("Cuvette_pump_set_flowrate interpretation failed");
                return false;
            }

            Logger::Debug("Cuvette pump flowrate set to: {:03.1f}", set_flowrate.flowrate);
            Flowrate(set_flowrate.flowrate);
            return true;
        }

        case Codes::Message_type::Cuvette_pump_get_flowrate_request: {
            App_messages::Cuvette_pump::Get_flowrate_response flowrate_response(Flowrate());
            Logger::Debug("Cuvette pump flowrate requested, response: {:03.1f}", flowrate_response.flowrate);
            Send_CAN_message(flowrate_response);
            return true;
        }

        case Codes::Message_type::Cuvette_pump_set_max_flowrate: {
            App_messages::Cuvette_pump::Set_max_flowrate set_max_flowrate;

            if (!set_max_flowrate.Interpret_data(message.data)) {
                Logger::Error("Cuvette_pump_set_max_flowrate interpretation failed");
                return false;
            }

            Logger::Debug("Cuvette pump maximal flowrate set to: {:03.1f}", set_max_flowrate.flowrate);
            Set_Maximal_flowrate(set_max_flowrate.flowrate);
            return true;
        }

        case Codes::Message_type::Cuvette_pump_move: {
            App_messages::Cuvette_pump::Move move_message;

            if (!move_message.Interpret_data(message.data)) {
                Logger::Error("Cuvette_pump_move interpretation failed");
                return false;
            }

            Logger::Debug("Cuvette pump moving, volume: {:03.1f}, flowrate: {:03.1f}", move_message.volume, move_message.flowrate);
            Move(move_message.volume, move_message.flowrate);
            return true;
        }

        case Codes::Message_type::Cuvette_pump_stop: {
            Logger::Debug("Cuvette pump stop requested");
            Stop();
            return true;
        }

        case Codes::Message_type::Cuvette_pump_prime: {
            Logger::Debug("Cuvette pump prime requested");
            Prime();
            return true;
        }

        case Codes::Message_type::Cuvette_pump_purge: {
            Logger::Debug("Cuvette pump purge requested");
            Purge();
            return true;
        }

        case Codes::Message_type::Cuvette_pump_info_request: {
            Logger::Debug("Cuvette pump info request");
            App_messages::Cuvette_pump::Info_response response(Minimal_flowrate(), Maximal_flowrate());
            Send_CAN_message(response);
            return true;
        }

        default:
            return false;
    }
}

void Cuvette_pump::Stop(){
    pump_stopper->Abort();
    DC_HBridge::Stop();
}

void Cuvette_pump::Speed(float pump_speed) {
    float in = std::clamp(pump_speed, -1.0f, 1.0f);
    if (abs(in) < 1e-4f) {
        Stop();
        return;
    }

    float magnitude = motor_pump_speed_curve.To_speed(abs(in));
    float signed_speed = (in >= 0.0f) ? magnitude : -magnitude;
    float clamped = std::clamp(signed_speed, -1.0f, 1.0f);
    Logger::Debug("Pump_speed in={:0.3f} mapped={:0.3f}", in, magnitude);
    DC_HBridge::Speed(clamped);
}

float Cuvette_pump::Speed() {
    float motor_speed = DC_HBridge::Speed();
    float pump_speed = motor_pump_speed_curve.To_rate(abs(motor_speed));
    return (motor_speed >= 0.0f) ? pump_speed : -pump_speed;
}

float Cuvette_pump::Flowrate(float flowrate){
    float pump_speed = flowrate / Maximal_flowrate();
    Speed(pump_speed);
    return pump_speed;
}

float Cuvette_pump::Flowrate(){
    float speed = Speed();
    float direction = speed >= 0.0f ? 1.0f : -1.0f;
    float flowrate = (motor_pump_speed_curve.To_rate(abs(speed))) * Maximal_flowrate();
    flowrate *= direction;
    return flowrate;
}

float Cuvette_pump::Set_Maximal_flowrate(float flowrate){
    memory->Write_Cuvette_pump_max_flowrate(flowrate);
    Logger::Debug("Actual max flowrate: {:f}, New max flowrate: {:f}", max_flowrate, flowrate);
    max_flowrate = flowrate;
    return flowrate;
}

float Cuvette_pump::Move(float volume_ml){
    return Move(volume_ml, Maximal_flowrate());
}

float Cuvette_pump::Move(float volume_ml, float flowrate){
    // Limit effective flow rate to non-zero positive value lower them max flowrate of pump
    float effective_flowrate = std::clamp(flowrate, Minimal_flowrate(), Maximal_flowrate());

    Logger::Debug("Max flowrate: {:03.1f}, selected_flowrate: {:03.1f}", Maximal_flowrate(), flowrate);

    // Calculate time of pumping and set stopper executioner
    float pump_time_sec = (std::abs(volume_ml) / effective_flowrate) * 60;

    Logger::Debug("Pumping time: {:03.1f}, effective_flowrate: {:03.1f}", pump_time_sec, effective_flowrate);

    if (volume_ml > 0) {
        Flowrate(effective_flowrate);
    } else {
        Flowrate(-effective_flowrate);
    }

    pump_stopper->Execute(pump_time_sec * 1000);

    return pump_time_sec;
}

float Cuvette_pump::Prime(){
    return Move(cuvette_system_volume);
}

float Cuvette_pump::Purge(){
    return Move(-cuvette_system_volume);
}
