#include "mixer.hpp"

Mixer::Mixer(uint8_t pwm_pin, RPM_counter* tacho, float frequency, float max_rpm, float min_speed):
    Component(Codes::Component::Bottle_mixer),
    Message_receiver(Codes::Component::Bottle_mixer),
    Fan_RPM(new PWM(pwm_pin, frequency, 0.0f, true), tacho),
    max_rpm(max_rpm),
    min_speed(min_speed)
{
    auto stopper_lamda = [this](){
          Stop();
    };

    mixer_stopper = new rtos::Delayed_execution(stopper_lamda);
}

bool Mixer::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Mixer::Receive(Application_message message){
    switch (message.Message_type()) {
        case Codes::Message_type::Mixer_set_speed: {
            App_messages::Mixer::Set_speed set_speed;

            if (!set_speed.Interpret_data(message.data)) {
                Logger::Print("Mixer_set_speed interpretation failed");
                return false;
            }

            Logger::Print(emio::format("Mixer speed set to: {:03.1f}", set_speed.speed));
            Speed(set_speed.speed);
            return true;
        }

        case Codes::Message_type::Mixer_get_speed_request: {
            App_messages::Mixer::Get_speed_response speed_response(Speed());
            Logger::Print(emio::format("Mixer speed requested, response: {:03.1f}", speed_response.speed));
            Send_CAN_message(speed_response);
            return true;
        }

        case Codes::Message_type::Mixer_set_rpm: {
            App_messages::Mixer::Set_rpm set_rpm;

            if (!set_rpm.Interpret_data(message.data)) {
                Logger::Print("Mixer_set_rpm interpretation failed");
                return false;
            }

            Logger::Print(emio::format("Mixer RPM set to: {:04.1f}", set_rpm.rpm));
            RPM(set_rpm.rpm);
            return true;
        }

        case Codes::Message_type::Mixer_get_rpm_request: {
            App_messages::Mixer::Get_rpm_response rpm_response(RPM());
            Logger::Print(emio::format("Mixer RPM requested, response: {:03.1f}", rpm_response.rpm));
            Send_CAN_message(rpm_response);
            return true;
        }

        case Codes::Message_type::Mixer_stir: {
            App_messages::Mixer::Stir stir_message;

            if (!stir_message.Interpret_data(message.data)) {
                Logger::Print("Mixer_stir interpretation failed");
                return false;
            }

            Logger::Print(emio::format("Mixer stirring, rpm: {:03.1f}, time: {:04.1f}s", stir_message.rpm, stir_message.time));
            Stir(stir_message.rpm, stir_message.time);
            return true;
        }

        case Codes::Message_type::Mixer_stop: {
            Logger::Print("Mixer stop requested");
            Stop();
            return true;
        }


        default:
            return false;
    }
}

float Mixer::Speed(float speed){
    float clamped_speed = std::clamp(speed, 0.0f, 1.0f);

    float limited_speed = 0;
    if(speed > 0){
        limited_speed = min_speed + clamped_speed * (1.0f - min_speed);
    }

    Intensity(limited_speed);

    return limited_speed;
}

float Mixer::Speed(){
    float speed = Intensity();
    return (speed - min_speed) / (1.0f - min_speed);
}

float Mixer::RPM(float rpm){
    float clamped_rpm = std::clamp(rpm, 0.0f, max_rpm);
    float speed = clamped_rpm / max_rpm;

    return Speed(speed);
}

void Mixer::Stir(float rpm, float time_s){
    RPM(rpm);
    mixer_stopper->Execute(time_s * 1000);
}

void Mixer::Stop(){
    mixer_stopper->Abort();
    Off();
}


