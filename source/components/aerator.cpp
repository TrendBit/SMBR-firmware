#include "aerator.hpp"

Aerator::Aerator(uint gpio_in1, uint gpio_in2, EEPROM_storage * const memory, float min_speed, float pwm_frequency) :
    Component(Codes::Component::Bottle_aerator),
    Message_receiver(Codes::Component::Bottle_aerator),
    DC_HBridge(gpio_in1, gpio_in2, pwm_frequency, DC_HBridge::Stop_mode::Brake),
    memory(memory)
{
    auto stopper_lamda = [this](){
          Stop();
    };
    pump_stopper = new rtos::Delayed_execution(stopper_lamda);
    Load_max_flowrate();
}

void Aerator::Load_max_flowrate(){
    std::optional<float> max_flowrate_mem_o = memory->Read_Aerator_max_flowrate();
    if (!max_flowrate_mem_o.has_value()) {
        max_flowrate = fallback_max_flowrate;
        return;
    }
    float max_flowrate_mem = max_flowrate_mem_o.value();
    if ((max_flowrate_mem >= Minimal_flowrate()) && (max_flowrate_mem <= 50000.0f)) {
        // Loaded float is in valid range
        max_flowrate = max_flowrate_mem;
    } else {
        max_flowrate = fallback_max_flowrate;
    }
    Logger::Notice("Aerator max flowrate: {:f}", max_flowrate);
}

bool Aerator::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Aerator::Receive(Application_message message){
    switch (message.Message_type()) {
        case Codes::Message_type::Aerator_set_speed: {
            App_messages::Aerator::Set_speed set_speed;

            if (!set_speed.Interpret_data(message.data)) {
                Logger::Error("Aerator_set_speed interpretation failed");
                return false;
            }

            Logger::Debug("Aerator speed set to: {:03.1f}", set_speed.speed);
            Speed(set_speed.speed);
            return true;
        }

        case Codes::Message_type::Aerator_get_speed_request: {
            App_messages::Aerator::Get_speed_response speed_response(Speed());
            Logger::Debug("Aerator pump speed requested, response: {:03.1f}", speed_response.speed);
            Send_CAN_message(speed_response);
            return true;
        }

        case Codes::Message_type::Aerator_set_flowrate: {
            App_messages::Aerator::Set_flowrate set_flowrate;

            if (!set_flowrate.Interpret_data(message.data)) {
                Logger::Error("Aerator_set_flowrate interpretation failed");
                return false;
            }

            Logger::Debug("Aerator pump flowrate set to: {:03.1f}", set_flowrate.flowrate);
            Flowrate(set_flowrate.flowrate);
            return true;
        }

        case Codes::Message_type::Aerator_get_flowrate_request: {
            App_messages::Aerator::Get_flowrate_response flowrate_response(Flowrate());
            Logger::Debug("Aerator flowrate requested, response: {:03.1f}", flowrate_response.flowrate);
            Send_CAN_message(flowrate_response);
            return true;
        }

        case Codes::Message_type::Aerator_set_max_flowrate: {
            App_messages::Aerator::Set_max_flowrate set_max_flowrate;

            if (!set_max_flowrate.Interpret_data(message.data)) {
                Logger::Error("Aerator_set_max_flowrate interpretation failed");
                return false;
            }

            Logger::Debug("Aerator maximal flowrate set to: {:03.1f}", set_max_flowrate.flowrate);
            Set_Maximal_flowrate(set_max_flowrate.flowrate);
            return true;
        }

        case Codes::Message_type::Aerator_move: {
            App_messages::Aerator::Move move_message;

            if (!move_message.Interpret_data(message.data)) {
                Logger::Error("Aerator_move interpretation failed");
                return false;
            }

            Logger::Debug("Aerator pump moving, volume: {:03.1f}, flowrate: {:03.1f}", move_message.volume, move_message.flowrate);
            Move(move_message.volume, move_message.flowrate);
            return true;
        }

        case Codes::Message_type::Aerator_stop: {
            Logger::Debug("Aerator pump stop requested");
            Stop();
            return true;
        }

        case Codes::Message_type::Aerator_info_request: {
            App_messages::Aerator::Info_response info_response(Minimal_flowrate(), Maximal_flowrate());
            Logger::Debug("Aerator pump info requested, response: min_flowrate: {:d}, max_flowrate: {:d}",
                          info_response.min_flowrate, info_response.max_flowrate);
            Send_CAN_message(info_response);
            return true;
        }

        default:
            return false;
    }
}

void Aerator::Speed(float pump_speed) {
    float in = std::clamp(pump_speed, 0.0f, 1.0f);
    if (in < 1e-4f) {
        Stop();
        return;
    }

    float magnitude = motor_pump_speed_curve.To_speed(in);
    float clamped = std::clamp(magnitude, 0.0f, 1.0f);
    Logger::Debug("Aerator_speed in={:0.3f} mapped={:0.3f}", in, magnitude);
    DC_HBridge::Speed(clamped);
}

float Aerator::Speed() {
    float motor_speed = DC_HBridge::Speed();
    float pump_speed = motor_pump_speed_curve.To_rate(motor_speed);
    return pump_speed;
}

float Aerator::Flowrate(float flowrate){
    float pump_speed = flowrate / Maximal_flowrate();
    Speed(pump_speed);
    return pump_speed;
}

float Aerator::Flowrate(){
    float speed = Speed();
    float flowrate = (motor_pump_speed_curve.To_rate(speed)) * Maximal_flowrate();
    return flowrate;
}

float Aerator::Set_Maximal_flowrate(float flowrate){
    memory->Write_Aerator_max_flowrate(flowrate);
    Logger::Debug("Actual max flowrate: {:f}, New max flowrate: {:f}", max_flowrate, flowrate);
    max_flowrate = flowrate;
    return flowrate;
}

float Aerator::Move(float volume_ml){
    return Move(volume_ml, Maximal_flowrate());
}

float Aerator::Move(float volume_ml, float flowrate){
    // Volume must be positive, pumping in only one direction
    if (volume_ml <= 0) {
        return 0;
    }

    // Limit effective flow rate to non-zero positive value lower them max flowrate of pump
    float effective_flowrate = std::clamp(flowrate, Minimal_flowrate(), Maximal_flowrate());

    Logger::Debug("Max flowrate: {:03.1f}, selected_flowrate: {:03.1f}", Maximal_flowrate(), flowrate);

    // Calculate time of pumping and set stopper executioner
    float pump_time_sec = (std::abs(volume_ml) / effective_flowrate) * 60;

    Logger::Debug("Pumping time: {:03.1f}, effective_flowrate: {:03.1f}", pump_time_sec, effective_flowrate);

    // Start pump with effective flowrate
    Flowrate(effective_flowrate);

    // Stop pump after calculated time
    pump_stopper->Execute(pump_time_sec * 1000);

    return pump_time_sec;
}

void Aerator::Stop(){
    pump_stopper->Abort();
    Brake();
}
