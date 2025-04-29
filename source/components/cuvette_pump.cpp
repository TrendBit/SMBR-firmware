#include "cuvette_pump.hpp"

Cuvette_pump::Cuvette_pump(uint gpio_in1, uint gpio_in2, float max_flowrate, float cuvette_system_volume, float min_speed, float pwm_frequency) :
    Component(Codes::Component::Cuvette_pump),
    Message_receiver(Codes::Component::Cuvette_pump),
    Peristaltic_pump(gpio_in1, gpio_in2, max_flowrate, min_speed, pwm_frequency),
    cuvette_system_volume(cuvette_system_volume)
{
    auto stopper_lamda = [this](){
          Stop();
    };
    pump_stopper = new rtos::Delayed_execution(stopper_lamda);
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

        default:
            return false;
    }
}

void Cuvette_pump::Stop(){
    pump_stopper->Abort();
    Coast();
}

float Cuvette_pump::Move(float volume_ml){
    return Move(volume_ml, max_flowrate);
}

float Cuvette_pump::Move(float volume_ml, float flowrate){
    // Limit effective flow rate to non-zero positive value lower them max flowrate of pump
    float effective_flowrate;

    if (flowrate <= 0) {
        effective_flowrate = max_flowrate;
    } else {
        effective_flowrate = std::clamp(flowrate, 0.0f, max_flowrate);
    }

    Logger::Debug("Max flowrate: {:03.1f}, selected_flowrate: {:03.1f}", max_flowrate, flowrate);

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
