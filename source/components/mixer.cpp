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

    gpio_set_slew_rate(pwm_pin, GPIO_SLEW_RATE_FAST);
    gpio_set_drive_strength(pwm_pin, GPIO_DRIVE_STRENGTH_8MA);

    mixer_stopper = new rtos::Delayed_execution(stopper_lamda);

    // Prepare regulation loop, do not start it until target temperature is set
    auto regulation_lambda = [this](){ this->Regulation_loop(); };
    regulation_loop = new rtos::Repeated_execution(regulation_lambda, 50, true);

    control = new qlibs::pidController();
    filter1 = new qlibs::smootherLPF2();
    filter2 = new qlibs::smootherLPF2();

    filter1->setup(0.2 );
    filter2->setup(0.6 );

    //control->setup(0.00003,0.000017,0.000001,0.05);
    control->setup(0.00015,0.0002,0.000000,0.05);
    control->setSaturation(0.00, 1.0);
}

void Mixer::Regulation_loop(){
    Logger::Trace("Mixer regulation loop");

    float real_rpm = Fan_RPM::RPM();

    float current_rpm_fast = filter2->smooth(real_rpm);

    float current_rpm_slow;

    if(real_rpm < 2){
        current_rpm_slow = current_rpm_fast;
    } else {
        current_rpm_slow = real_rpm;
    }

    current_rpm = filter1->smooth(current_rpm_slow);

    if(target_rpm > 0){
        float output = control->control(target_rpm, current_rpm);

        Speed(output);

        // Log detailed information
        Logger::Notice("Target={:4.0f}, Real={:4.0f}, Current={:4.0f}, CurrentF={:4.0f}, Output={:.3f}", target_rpm, real_rpm, current_rpm_slow, current_rpm, output);
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
            Logger::Notice("Mixer RPM requested, response: {:03.1f}", rpm_response.rpm);
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


