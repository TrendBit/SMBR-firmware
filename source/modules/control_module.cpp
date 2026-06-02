#include "control_module.hpp"
#include "threads/module_check_thread.hpp"
#include "module_check/led_temperature_check.hpp"
#include "module_check/board_temperature_check.hpp"
#include "module_check/core_temperature_check.hpp"
#include "module_check/core_load_check.hpp"
#include "module_check/heater_plate_temp_check.hpp"
#include "module_check/mixer_rpm_check.hpp"
#include <cstdint>

Control_module::Control_module():
    Base_module(
        Codes::Module::Control_module,
        new Enumerator(Codes::Module::Control_module,memory,Codes::Instance::Exclusive),
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
    Setup_module_check();
    Setup_cli_temps();
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
    cuvette_pump = new Cuvette_pump(22, 8, 20.0, memory, 0.2, 20.0f);
}

void Control_module::Setup_aerator(){
    Logger::Debug("Aerator initialization");
    aerator = new Aerator(3, 2, memory, 0.12, 50.0f);
}

void Control_module::Setup_mixer(){

    Logger::Debug("Mixer initialization");
    auto mixer_tacho = new RPM_counter_PIO(PIO_machine(pio0,1),7, 10000.0, 280,2);
    mixer = new Mixer(13, mixer_tacho, 8, 300.0, 6000.0);
}

void Control_module::Setup_module_check(){
    if (led_panel) {
        module_check_thread->AttachCheck(new Led_temperature_check(led_panel));
    }
    if (heater) {
        module_check_thread->AttachCheck(new Heater_plate_temp_check(heater));
    }
    if (mixer) {
        module_check_thread->AttachCheck(new Mixer_rpm_check(mixer));
    }
    module_check_thread->AttachCheck(new Board_temperature_check(this));
    module_check_thread->AttachCheck(new Core_temperature_check(common_core));
    module_check_thread->AttachCheck(new Core_load_check(common_core));
}

void Control_module::Setup_cli_temps(){
    if (led_panel) {
        register_temperature_readout("led_panel", [this]()->std::optional<float>{
            return std::optional<float>{this->led_panel->Temperature()};
        });
    }
    if (heater) {
        register_temperature_readout("heater", [this]()->std::optional<float>{
            return std::optional<float>{this->heater->Temperature()};
        });
    }
    
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


bool Parse_channel(const std::string& arg, CLI_service& cli, uint8_t& selected_channel){
    if(arg.size() != 1){
        cli.Print_ln(dye::red("invalid channel!"));
        return false;
    }
    
    auto arg_char = arg[0];

    if( arg_char < '0' || arg_char > '9'){
        cli.Print_ln(dye::red("invalid channel!"));
        return false;
    }
    
    selected_channel = arg_char - '0';
    return true;
}

bool Parse_channels(
    const std::vector<std::string>& args,
    CLI_service& cli, 
    bool fill_on_empty, 
    std::vector<uint8_t>& selected_channels,
    size_t skip_args = 0
){    
    
    const size_t channel_count = 10;
    if(args.size()==skip_args){
        if(fill_on_empty){
            selected_channels.reserve(channel_count);
            
            for(uint8_t i =0; i<channel_count; i++){
                selected_channels.push_back(i);
            }
        }else{
            cli.Print_ln(dye::red("missing channels"));
            return false;
        }
    }else{
        for (size_t i = skip_args; i < args.size(); i++){
            const auto& arg = args[i];
            uint8_t channel;
            
            if (not Parse_channel(arg, cli, channel)){
                return false;
            }
            
            if (channel > (channel_count - 1)){
                return false;
            }
            
            selected_channels.push_back(channel);
        }
    }
    return true;
}

void Control_module::Setup_cli(CLI_service& cli) const {
    if( led_panel ){
        cli.Bind("led_set_intensity",[this, &cli](std::vector<std::string> args){
            if(not CLI_service::Check_argument_count(args,cli,2)){
                return;
            }
            
            float intensity = 0.0;
            if(not CLI_service::Parse_argument(args[0],cli,intensity)){
                return;
            }
            
            
            std::vector<uint8_t> channels;
            if(not Parse_channels(args,cli,false,channels,1)){
                return;
            }
            
            for(const auto& channel : channels){
                if(led_panel->Set_intensity(channel, intensity)){
                    cli.Print_ln("success");
                }else{
                    cli.Print_error("Set_intensity failed");
                }
            }
        },"set intensity of selected channels","intensity(float) channel channel...");
        
        cli.Bind("led_get_intensity",[this, &cli](std::vector<std::string> args){
            if(not CLI_service::Check_argument_count(args,cli,0)){
                return;
            }
            
            std::vector<uint8_t> channels;
            if(not Parse_channels(args,cli,false,channels)){
                return;
            }
            
            for(const auto& channel : channels){
                std::optional<float> intensity = led_panel->Get_intensity(channel);
                
                if(intensity.has_value()){
                    cli.Print_ln(emio::format("channel {}: {}",channel, intensity));
                }else{
                    cli.Print_error("Get_intensity failed");
                }
            }
        },"set intensity of selected channels","channel channel...");
        
        cli.Bind("led_power_limited",[this, &cli](){
            if(led_panel->Power_limited()){
                cli.Print_ln("not limited");
            }else{
                cli.Print_ln(dye::yellow("limited"));
            }
        },"Detect if power of LED illumination is limited by power budget");
        
        cli.Bind("led_temp_limited",[this, &cli](){
            if(led_panel->Temperature_limited()){
                cli.Print_ln("not limited");
            }else{
                cli.Print_ln(dye::yellow("limited"));
            }
        },"Detect if power of LED illumination is limited by temperature of module");
    }
}