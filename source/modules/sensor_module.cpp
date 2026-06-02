#include "sensor_module.hpp"
#include "cli.hpp"
#include "codes/tools/magic_enum.hpp"
#include "components/spectrophotometer.hpp"
#include "fluorometer/fluorometer_config.hpp"
#include "threads/module_check_thread.hpp"
#include "module_check/board_temperature_check.hpp"
#include "module_check/core_temperature_check.hpp"
#include "module_check/core_load_check.hpp"
#include "module_check/bottle_temp_check.hpp"
#include "module_check/bottle_top_measured_temp_check.hpp"
#include "module_check/bottle_bottom_measured_temp_check.hpp"
#include "module_check/bottle_top_sensor_temp_check.hpp"
#include "module_check/bottle_bottom_sensor_temp_check.hpp"
#include "module_check/fluorometer_emitor_temp_check.hpp"
#include "module_check/fluorometer_detector_temp_check.hpp"
#include "module_check/spectrophotometer_emitor_temp_check.hpp"
#include "tools/color.hpp"

Sensor_module::Sensor_module():
    Base_module(
        Codes::Module::Sensor_module,
        new Enumerator(Codes::Module::Sensor_module,memory,Codes::Instance::Exclusive),
        24, 10, 11, 13),
    ntc_channel_selector(new GPIO(18, GPIO::Direction::Out)),
    ntc_thermistors(new Thermistor(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_3, 3.30f), 3950, 10000, 25, 5100)),
    cuvette_mutex(new fra::MutexStandard())
{
    Setup_components();
}

void Sensor_module::Setup_components(){
    Logger::Debug("Sensor module component setup");
    Setup_bottle_thermometers();
    Setup_Mini_OLED();
    Setup_fluorometer();
    Setup_spectrophotometer();
    Setup_module_check();
}

std::optional<float> Sensor_module::Board_temperature(){
    bool lock = adc_mutex->Lock(0);
    if (!lock) {
        Logger::Warning("Board temp ADC mutex lock failed");
        return std::nullopt;
    }
    ntc_channel_selector->Set(true);
    float temp = ntc_thermistors->Temperature();
    adc_mutex->Unlock();
    return temp;
}

void Sensor_module::Setup_Mini_OLED(){
    Logger::Debug("Setting up Mini OLED");
    mini_oled = new Mini_OLED(bottle_temperature, 10);
}

void Sensor_module::Setup_bottle_thermometers(){
    Logger::Debug("Setting up bottle thermometers");

    TLA2024 *adc = new TLA2024(*i2c, 0x4b);

    TLA2024_channel *adc_ch0 = new TLA2024_channel(adc, TLA2024::Channels::AIN0_GND);
    TLA2024_channel *adc_ch1 = new TLA2024_channel(adc, TLA2024::Channels::AIN1_GND);
    TLA2024_channel *adc_ch2 = new TLA2024_channel(adc, TLA2024::Channels::AIN2_GND);
    TLA2024_channel *adc_ch3 = new TLA2024_channel(adc, TLA2024::Channels::AIN3_GND);

    Thermopile *thermopile_top    = new Thermopile(adc_ch1, adc_ch0, 0.95);
    Thermopile *thermopile_bottom = new Thermopile(adc_ch3, adc_ch2, 0.95);

    bottle_temperature = new Bottle_temperature(thermopile_top, thermopile_bottom);
}

void Sensor_module::Setup_fluorometer(){
    Logger::Debug("Setting up fluorometer");
    auto led_pwm = new PWM_channel(23, 1000000, 0.0, true);
    uint detector_gain_selector_pin = 21;

    fluorometer = new Fluorometer(led_pwm, detector_gain_selector_pin, ntc_channel_selector, ntc_thermistors, i2c, memory, cuvette_mutex, adc_mutex);
}

void Sensor_module::Setup_spectrophotometer(){
    spectrophotometer = new Spectrophotometer(*i2c, memory, cuvette_mutex);
}

void Sensor_module::Setup_module_check(){
    module_check_thread->AttachCheck(new Board_temperature_check(this));
    module_check_thread->AttachCheck(new Core_temperature_check(common_core));
    module_check_thread->AttachCheck(new Core_load_check(common_core));
    if (bottle_temperature) {
        module_check_thread->AttachCheck(new Bottle_temp_check(bottle_temperature));
        module_check_thread->AttachCheck(new Bottle_top_measured_temp_check(bottle_temperature));
        module_check_thread->AttachCheck(new Bottle_bottom_measured_temp_check(bottle_temperature));
        module_check_thread->AttachCheck(new Bottle_top_sensor_temp_check(bottle_temperature));
        module_check_thread->AttachCheck(new Bottle_bottom_sensor_temp_check(bottle_temperature));
    }
    if (fluorometer) {
        module_check_thread->AttachCheck(new Fluorometer_emitor_temp_check(fluorometer));
        module_check_thread->AttachCheck(new Fluorometer_detector_temp_check(fluorometer));
    }
    if (spectrophotometer) {
        module_check_thread->AttachCheck(new Spectrophotometer_emitor_temp_check(spectrophotometer));
    }
}

void Sensor_module::Setup_cli_temps(){
    if (bottle_temperature) {
        register_temperature_readout("bottle", [this]()->std::optional<float>{
            return std::optional<float>{this->bottle_temperature->Temperature()};
        });
        
        register_temperature_readout("bottle_top", [this]()->std::optional<float>{
            return std::optional<float>{this->bottle_temperature->Top_temperature()};
        });
        
        register_temperature_readout("bottle_sensor_top", [this]()->std::optional<float>{
            return std::optional<float>{this->bottle_temperature->Top_sensor_temperature()};
        });
        
        register_temperature_readout("bottle_bottom", [this]()->std::optional<float>{
            return std::optional<float>{this->bottle_temperature->Bottom_temperature()};
        });
        
        register_temperature_readout("bottle_sensor_bottom", [this]()->std::optional<float>{
            return std::optional<float>{this->bottle_temperature->Bottom_sensor_temperature()};
        });
    }
    
    if (fluorometer) {
        register_temperature_readout("fluorometer_emitor", [this]()->std::optional<float>{
            return std::optional<float>{this->fluorometer->Emitor_temperature()};
        });
        register_temperature_readout("fluorometer_detector", [this]()->std::optional<float>{
            return std::optional<float>{this->fluorometer->Detector_temperature()};
        });
    }
    
    if (spectrophotometer) {
        register_temperature_readout("spectrophotometer", [this]()->std::optional<float>{
            return std::optional<float>{this->spectrophotometer->Temperature()};
        });
    }
}


bool Parse_channels(
    const std::vector<std::string>& args,
    CLI_service& cli, 
    bool fill_on_empty, 
    std::vector<Spectrophotometer::Channels>& selected_channels,
    size_t skip_args = 0
){
    if(args.size()==skip_args){
        if(fill_on_empty){
            std::vector<Spectrophotometer::Channels> all_channels{
                Spectrophotometer::Channels::UV,
                Spectrophotometer::Channels::Blue,
                Spectrophotometer::Channels::Green,
                Spectrophotometer::Channels::Orange,
                Spectrophotometer::Channels::Red,
                Spectrophotometer::Channels::IR
            };
            selected_channels = std::move(all_channels);
        }else{
            cli.Print_error("missing channels");
            return false;
        }
    }else{
        for (size_t i = skip_args; i < args.size(); i++){
            const auto& arg = args[i];
            Spectrophotometer::Channels channel;
            
            if(arg == "UV"){
                channel = Spectrophotometer::Channels::UV;
            }else if(arg == "Blue"){
                channel = Spectrophotometer::Channels::Blue;
            }else if(arg == "Green"){
                channel = Spectrophotometer::Channels::Green;
            }else if(arg == "Orange"){
                channel = Spectrophotometer::Channels::Orange;
            }else if(arg == "Red"){
                channel = Spectrophotometer::Channels::Red;
            }else if(arg == "IR"){
                channel = Spectrophotometer::Channels::IR;
            }else{
                cli.Print_error("invalid channel");
                return false;
            }
            
            selected_channels.push_back(channel);
        }
    }
    return true;
}

void Sensor_module::Setup_cli(CLI_service& cli) const {
    if(fluorometer){
        cli.Bind("fluorometer_capture",[this, &cli](std::vector<std::string> args){
            if( not CLI_service::Check_argument_count(args, cli, 3, 5)){
                return;
            }
            
            Fluorometer_config::Gain gain;
            const auto& gain_arg = args[0];
            
            if(gain_arg == "x1"){
                gain = Fluorometer_config::Gain::x1;
            }else if(gain_arg == "x10"){
                gain = Fluorometer_config::Gain::x10;
            }else if(gain_arg == "x50"){
                gain = Fluorometer_config::Gain::x50;
            }else if(gain_arg == "Auto"){
                gain = Fluorometer_config::Gain::Auto;
            }else{
                cli.Print_error("Invalid gain selected");
                return;
            }
            
            float intensity = 0.0;
            float capture_length = 0.0;
            if( not CLI_service::Parse_argument(args[1],cli,intensity)
            ||  not CLI_service::Parse_argument(args[2],cli,capture_length)
            ){
                return;
            }
            
            // samples defined
            if(args.size() == 4){
                uint samples = 0;
                if( not CLI_service::Parse_argument(args[3],cli,samples)){
                    return;
                }
                
                if(fluorometer->Capture_OJIP(gain, intensity, capture_length, samples)){
                    cli.Print_ln("success");
                }else{
                    cli.Print_error("Capture_OJIP failed");
                }
                
                return;
            }
            
            // samples and timing defined
            if(args.size() == 5){
                uint samples = 0;
                if( not CLI_service::Parse_argument(args[3],cli,samples)){
                    return;
                }
                Fluorometer_config::Timing timing;
                const auto& timing_arg = args[4];
                
                if(timing_arg == "Linear"){
                    timing = Fluorometer_config::Timing::Linear;
                }else if(timing_arg == "Logarithmic"){
                    timing = Fluorometer_config::Timing::Logarithmic;
                }else{
                    cli.Print_error("Invalid timing selected");
                    return;
                }
                
                if(fluorometer->Capture_OJIP(gain, intensity, capture_length, samples, timing)){
                    cli.Print_ln("success");
                }else{
                    cli.Print_error("Capture_OJIP failed");
                }
                
                return;
            }
            
            if(fluorometer->Capture_OJIP(gain, intensity, capture_length)){
                cli.Print_ln("success");
            }else{
                cli.Print_error("Capture_OJIP failed");
            }
            
        },"create an OJIP capture","gain emitor_intesity(float) capture_length(float) [samples(int)]? [timing]?");
        
        cli.Bind("fluorometer_check",[this, &cli](){
            if(fluorometer->Capture_done()){
                cli.Print_ln("done");
            }else{
                cli.Print_ln(dye::yellow("in progress"));
            }
            
        },"check if the fluorometer capture is complete");
        
        cli.Bind("fluorometer_retrieve",[this, &cli](){
            if(fluorometer->Capture_done()){
                cli.Print_ln("TODO"); //#TODO how to export this?
            }else{
                cli.Print_ln(dye::yellow("in progress"));
            }
        },"retrieve the last capture data");
    }
    
    if(spectrophotometer){
        cli.Bind("spectrophotometer_measure",[this, &cli](std::vector<std::string> args){
            if(not CLI_service::Check_argument_count(args, cli, 0)){
                return;
            }
            
            std::vector<Spectrophotometer::Channels> channels;
            if (not Parse_channels(args, cli, true, channels)){
                return;
            }
            
            for(const auto& channel : channels){
                auto measurement = spectrophotometer->Measure_channel(channel);
                cli.Print_ln(
                    emio::format("{}: {}  {}%",
                        magic_enum::enum_name(channel),
                        measurement.absolute_value,
                        measurement.relative_value
                ));
            }
        },"measure all, or selected channels","[channel channel ...]?");
        
        cli.Bind("spectrophotometer_measure_intensity",[this, &cli](std::vector<std::string> args){
            if(not CLI_service::Check_argument_count(args, cli, 0)){
                return;
            }
            
            std::vector<Spectrophotometer::Channels> channels;
            if (not Parse_channels(args, cli, true, channels)){
                return;
            }
            
            for(const auto& channel : channels){
                auto measurement = spectrophotometer->Measure_intensity(channel);
                cli.Print_ln(
                    emio::format("{}: {}%",
                        magic_enum::enum_name(channel),
                        measurement
                ));
            }
        },"measure all, or selected channels intensity","[channel channel ...]?");
    }
}