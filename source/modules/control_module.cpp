#include "control_module.hpp"

Control_module::Control_module():
    Base_module(
        Codes::Module::Control_module,
        new Enumerator(Codes::Module::Control_module ,Codes::Instance::Exclusive),
        24, 18, 19),
    board_thermistor(new Thermistor(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_1, 3.30f), 3950, 10000, 25, 5100))
{
    Setup_components();
}

void Control_module::Setup_components(){

    PWM_channel * case_fan = new PWM_channel(12, 100, 1.0, true);

    Setup_LEDs();
    Setup_heater();
    Setup_cuvette_pump();
    Setup_aerator();
    Setup_mixer();
}

void Control_module::Setup_LEDs(){
    Logger::Debug("LED initialization");

    PWM_channel * r_pwm = new PWM_channel(17, 100, 0.00, true);
    PWM_channel * g_pwm = new PWM_channel(16, 100, 0.00, true);
    PWM_channel * b_pwm = new PWM_channel(14, 100, 0.00, true);
    PWM_channel * w_pwm = new PWM_channel(15, 100, 0.00, true);

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
    Logger::Debug("Heater initialization");

    GPIO * heater_vref = new GPIO(20, GPIO::Direction::Out);
    heater_vref->Set(true);

    // 8W power (frequency 100 kHz): cooling -0.77, heating 0.75
    heater = new Heater(23, 25, 400000);
}

void Control_module::Setup_cuvette_pump(){
    Logger::Debug("Cuvette_pump initialization");
    PWM_channel * cuvettte_pump_vref_pwm = new PWM_channel(10, 2000, 0.2, true);
    cuvette_pump = new Cuvette_pump(22, 8, 100.0, 20.0, 0.2, 250.0f);
}

void Control_module::Setup_aerator(){
    Logger::Debug("Aerator initialization");
    aerator = new Aerator(3, 2, 2500.0, 0.12, 50.0f);
}

void Control_module::Setup_mixer(){

    Logger::Debug("Mixer initialization");
    auto mixer_tacho = new RPM_counter_PIO(PIO_machine(pio0,1),7, 10000.0, 280,2);
    mixer = new Mixer(13, mixer_tacho, 8, 300.0, 6000.0);
}

std::optional<float> Control_module::Board_temperature(){
    bool lock = adc_mutex->Lock(0);
    if (!lock) {
        Logger::Warning("Board temp ADC mutex lock failed");
        return std::nullopt;
    }
    float temp = board_thermistor->Temperature();
    adc_mutex->Unlock();
    return temp;
}
