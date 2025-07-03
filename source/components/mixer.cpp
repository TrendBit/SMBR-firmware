#include "mixer.hpp"

Mixer::Mixer(uint8_t pwm_pin, RPM_counter* tacho, float frequency, float min_rpm, float max_rpm):
    Component(Codes::Component::Bottle_mixer),
    Message_receiver(Codes::Component::Bottle_mixer),
    Fan_RPM(new PWM_channel(pwm_pin, frequency, 0.0f, true), tacho),
    min_rpm(min_rpm),
    max_rpm(max_rpm)
{
    auto stopper_lamda = [this](){
          Stop();
    };

    rpm_filter = new Mixer_rpm_filter();

    gpio_set_slew_rate(pwm_pin, GPIO_SLEW_RATE_FAST);
    gpio_set_drive_strength(pwm_pin, GPIO_DRIVE_STRENGTH_8MA);

    mixer_stopper = new rtos::Delayed_execution(stopper_lamda);

    // Prepare regulation loop, do not start it until target temperature is set
    auto regulation_lambda = [this](){ this->Regulation_loop(); };
    regulation_loop = new rtos::Repeated_execution(regulation_lambda, 125, true);

    control = new qlibs::pidController();

    control->setup(0.002,0.0001,0.0,0.125);
    control->setSaturation(0.05, 1.0);
}

void Mixer::Regulation_loop(){
    Logger::Trace("Mixer regulation loop");

    float real_rpm = Fan_RPM::RPM();

    current_rpm = rpm_filter->Smooth(real_rpm);

    if(target_rpm > 0){
        float output = control->control(target_rpm, current_rpm);

        Speed(output);

        // Logger::Notice("Target={:4.0f}, Real={:4.0f}, Filter={:4.0f}, Output={:.3f}", target_rpm, real_rpm, current_rpm, output);
    }
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

        case Codes::Message_type::Mixer_info_request: {
            App_messages::Mixer::Info_response info_response(Min_speed(), Max_speed());
            Logger::Debug("Mixer info requested, response: min={:d}, max={:d}", info_response.min_rpm, info_response.max_rpm);
            Send_CAN_message(info_response);
            return true;
        }

        default:
            return false;
    }
}

float Mixer::Speed(float speed){
    float limited_speed = std::clamp(speed, 0.0f, 1.0f);
    Intensity(limited_speed);
    return limited_speed;
}

float Mixer::Speed(){
    return Intensity();
}

float Mixer::RPM(float rpm){
    target_rpm = rpm;
    return target_rpm;
}

float Mixer::RPM(){
    return current_rpm;
}

void Mixer::Stir(float rpm, float time_s){
    RPM(rpm);
    mixer_stopper->Execute(time_s * 1000);
}

void Mixer::Stop(){
    target_rpm = 0;
    mixer_stopper->Abort();
    Off();
}


