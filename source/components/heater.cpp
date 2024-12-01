#include "heater.hpp"

Heater::Heater(uint gpio_in1, uint gpio_in2, float pwm_frequency):
    Component(Codes::Component::Bottle_heater),
    Message_receiver(Codes::Component::Bottle_heater),
    control_bridge(new DC_HBridge(new PWM(gpio_in1, pwm_frequency, 0.0, true), new PWM(gpio_in2, pwm_frequency, 0.0, true)))
{
    control_bridge->Coast();
}

float Heater::Intensity(float requested_intensity){
    this->intensity = requested_intensity; //std::clamp(requested_intensity, -1.0f, 1.0f);
    control_bridge->Speed(intensity);
    return intensity;
}

bool Heater::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Heater::Receive(Application_message message){
    return false;
}
