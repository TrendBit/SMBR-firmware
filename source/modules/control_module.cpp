#include "control_module.hpp"

Control_module::Control_module():
    Base_module(Codes::Module::Control_module, Codes::Instance::Exclusive, 24)
{
    Setup_components();
}

void Control_module::Setup_components(){
    Setup_LEDs();
    Setup_heater();

    GPIO * case_fan = new GPIO(12, GPIO::Direction::Out);
    case_fan->Set(false);

    GPIO * heater_fan = new GPIO(11, GPIO::Direction::Out);
    heater_fan->Set(false);

    GPIO * mixer_fan = new GPIO(13, GPIO::Direction::Out);
    mixer_fan->Set(false);
}

void Control_module::Setup_LEDs(){
    Logger::Print("LED initialization");

    PWM * r_pwm = new PWM(17, 100, 0.00, true);
    PWM * g_pwm = new PWM(16, 100, 0.00, true);
    PWM * b_pwm = new PWM(14, 100, 0.00, true);
    PWM * w_pwm = new PWM(15, 100, 0.00, true);

    // Each channel is limited to max 25% of power to keep LED relatively cool is conserve power budget
    LED_PWM * led_r = new LED_PWM(r_pwm, 0.01, 0.25, 10.0);
    LED_PWM * led_g = new LED_PWM(g_pwm, 0.01, 0.25, 10.0);
    LED_PWM * led_b = new LED_PWM(b_pwm, 0.01, 0.25, 10.0);
    LED_PWM * led_w = new LED_PWM(w_pwm, 0.01, 0.25, 10.0);

    led_r->Intensity(0.0);
    led_g->Intensity(0.0);
    led_b->Intensity(0.0);
    led_w->Intensity(0.0);

    auto temp_0_adc = new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_2, 3.30f);
    auto temp_0 = new Thermistor(temp_0_adc, 3950, 10000, 25, 5100);

    std::vector<LED_intensity *> led_channels = {led_r, led_g, led_b, led_w};

    led_panel = new LED_panel(led_channels, temp_0, 10.0);
}

void Control_module::Setup_heater(){
    Logger::Print("Heater initialization");

    GPIO * heater_vref = new GPIO(20, GPIO::Direction::Out);
    heater_vref->Set(true);
    heater = new Heater(23, 25, 20);
    // Limit max heater power to 0.3 for 10W limit, 0.75 for 25W limit
    heater->Intensity(0.0);

    // GPIO * in1 = new GPIO(23, GPIO::Direction::Out);
    // GPIO * in2 = new GPIO(25, GPIO::Direction::Out);

    // in1->Set(false);
    // in2->Set(true);
}
