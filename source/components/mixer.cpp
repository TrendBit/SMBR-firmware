#include "mixer.hpp"

Mixer::Mixer(uint8_t pwm_pin, RPM_counter* tacho, float frequency, float max_rpm, float min_speed):
    Component(Codes::Component::Bottle_mixer),
    Message_receiver(Codes::Component::Bottle_mixer),
    Fan_RPM(new PWM_channel(pwm_pin, frequency, 0.0f, true), tacho),
    max_rpm(max_rpm),
    min_speed(min_speed)
{
    auto stopper_lamda = [this](){
          Stop();
    };

    mixer_stopper = new rtos::Delayed_execution(stopper_lamda);

    // Prepare regulation loop, do not start it until target temperature is set
    auto regulation_lambda = [this](){ this->Regulation_loop(); };
    regulation_loop = new rtos::Repeated_execution(regulation_lambda, 500, false);
}

void Mixer::Regulation_loop(){
    Logger::Trace("Mixer regulation loop");

    float current_rpm = RPM();

    int target_error = static_cast<int>(target_rpm - current_rpm);

    float current_speed = Speed();

    float speed_adjustment = target_error * p_gain / max_rpm;

    float new_speed = current_speed + speed_adjustment;

    new_speed = std::clamp(new_speed, 0.0f, 1.0f);

    Speed(new_speed);

    // Log detailed information
    Logger::Debug("Target={:4.0f}, Current={:4.0f}, Error={:4d}, Previous={:.3f}, Adjustment={:.3f}, New={:.3f}", target_rpm, current_rpm, target_error, current_speed, speed_adjustment, new_speed);
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
                Logger::Error("Mixer_set_speed interpretation failed");
                return false;
            }

            Logger::Debug("Mixer speed set to: {:03.1f}", set_speed.speed);
            Speed(set_speed.speed);
            return true;
        }

        case Codes::Message_type::Mixer_get_speed_request: {
            App_messages::Mixer::Get_speed_response speed_response(Speed());
            Logger::Debug("Mixer speed requested, response: {:03.1f}", speed_response.speed);
            Send_CAN_message(speed_response);
            return true;
        }

        case Codes::Message_type::Mixer_set_rpm: {
            App_messages::Mixer::Set_rpm set_rpm;

            if (!set_rpm.Interpret_data(message.data)) {
                Logger::Error("Mixer_set_rpm interpretation failed");
                return false;
            }

            Logger::Debug("Mixer RPM set to: {:04.1f}", set_rpm.rpm);
            RPM(set_rpm.rpm);
            return true;
        }

        case Codes::Message_type::Mixer_get_rpm_request: {
            App_messages::Mixer::Get_rpm_response rpm_response(RPM());
            Logger::Debug("Mixer RPM requested, response: {:03.1f}", rpm_response.rpm);
            Send_CAN_message(rpm_response);
            return true;
        }

        case Codes::Message_type::Mixer_stir: {
            App_messages::Mixer::Stir stir_message;

            if (!stir_message.Interpret_data(message.data)) {
                Logger::Error("Mixer_stir interpretation failed");
                return false;
            }

            Logger::Debug("Mixer stirring, rpm: {:03.1f}, time: {:04.1f}s", stir_message.rpm, stir_message.time);
            Stir(stir_message.rpm, stir_message.time);
            return true;
        }

        case Codes::Message_type::Mixer_stop: {
            Logger::Debug("Mixer stop requested");
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
    target_rpm = rpm;

    float startup_speed = 0.2;

    regulation_loop->Enable();

    return Speed(startup_speed);
}

void Mixer::Stir(float rpm, float time_s){
    RPM(rpm);
    mixer_stopper->Execute(time_s * 1000);
}

void Mixer::Stop(){
    target_rpm = 0;
    mixer_stopper->Abort();
    regulation_loop->Disable();
    Off();
}


