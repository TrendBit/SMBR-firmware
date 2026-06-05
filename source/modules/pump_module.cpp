#include "pump_module.hpp"
#include "cli.hpp"
#include "tools/color.hpp"
#include <cstdint>

Pump_module::Pump_module():
    Base_module(
        Codes::Module::Pump_module,
        new Enumerator(
            Codes::Module::Pump_module,
            memory,
            Codes::Instance::Undefined,
            enumeration_button_gpio,
            enumeration_rgb_led_gpio),
        activity_green_led_gpio, i2c_sda_gpio, i2c_scl_gpio),
    board_thermistor(new Thermistor(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_1, 3.30f), 3950, 10000, 25, 5100))
{
    Setup_components();
}

void Pump_module::Setup_components(){
    Logger::Debug("Pump module component setup");

    uint8_t pump_count;
    // Detect number of pumps based on configuration pin
    GPIO config_pin = GPIO(config_detection_pin);
    config_pin.Set_direction(GPIO::Direction::In);
    config_pin.Set_pulls(true, false);
    if (config_pin.Read()) {
        pump_count = 4;
    } else {
        pump_count = 2;
    }

    Logger::Notice("Module configuration: {} pumps", pump_count);

    auto pump_adc = new TLA2024(*i2c, 0x48);

    auto adc_channel_1 = new TLA2024_channel(pump_adc, TLA2024::Channels::AIN2_GND);
    auto adc_channel_2 = new TLA2024_channel(pump_adc, TLA2024::Channels::AIN0_GND);
    auto adc_channel_3 = new TLA2024_channel(pump_adc, TLA2024::Channels::AIN3_GND);
    auto adc_channel_4 = new TLA2024_channel(pump_adc, TLA2024::Channels::AIN1_GND);

    auto current_sensor_1 = std::make_unique<Current_sensor>(adc_channel_3, 0.1f);
    auto current_sensor_2 = std::make_unique<Current_sensor>(adc_channel_1, 0.1f);
    auto current_sensor_3 = std::make_unique<Current_sensor>(adc_channel_4, 0.1f);
    auto current_sensor_4 = std::make_unique<Current_sensor>(adc_channel_2, 0.1f);

    // TODO, some pumps must be inverted in order to have correct direction (based on device case)
    auto pump_1 = new Pump( 6,  7, 16, std::move(current_sensor_3), 28.0f, 0.1f);
    auto pump_2 = new Pump(15, 14, 17, std::move(current_sensor_1), 28.0f, 0.1f);
    auto pump_3 = new Pump( 3,  2, 20, std::move(current_sensor_4), 28.0f, 0.1f);
    auto pump_4 = new Pump(11, 10, 21, std::move(current_sensor_2), 28.0f, 0.1f);

    pump_controller = new Pump_controller(
        pump_count == 2 ?
            etl::vector<Pump *,8>{ pump_1, pump_2 } :
            etl::vector<Pump *,8>{ pump_1, pump_2, pump_3, pump_4 },
        memory
    );

    Logger::Warning("No components to setup");
}

std::optional<float> Pump_module::Board_temperature(){
    bool lock = adc_mutex->Lock(0);
    if (!lock) {
        Logger::Warning("Board temp ADC mutex lock failed");
        return std::nullopt;
    }
    float temp = board_thermistor->Temperature();
    adc_mutex->Unlock();
    return temp;
}

/**
 * @brief parses 
 */
bool Parse_pump_index(const std::string& arg, uint8_t pump_count, CLI_service& cli, uint8_t& selected_pump_index){
    if(arg.size() != 1){
        cli.Print_error("invalid pump index!");
        return false;
    }
    
    auto arg_char = arg[0];

    if( arg_char <= '0' || arg_char > ('0' + pump_count)){
        cli.Print_error("pump index out of range!");
        return false;
    }
    
    selected_pump_index = arg_char - '0';
    return true;
}

bool Parse_pump_indexes(
    const std::vector<std::string>& args,
    uint8_t pump_count, 
    CLI_service& cli, 
    bool fill_on_empty, 
    std::vector<uint8_t>& selected_pump_indexes,
    size_t skip_args = 0
){
    if(args.size()==skip_args){
        if(fill_on_empty){
            selected_pump_indexes.reserve(pump_count);
            
            for(uint8_t i =0; i<pump_count; i++){
                selected_pump_indexes.push_back(i+1);
            }
        }else{
            cli.Print_error("missing pump indexes");
            return false;
        }
    }else{
        for (size_t i = skip_args; i < args.size(); i++){
            const auto& arg = args[i];
            uint8_t pump_index;
            
            if (not Parse_pump_index(arg, pump_count, cli, pump_index)){
                return false;
            }
            
            selected_pump_indexes.push_back(pump_index);
        }
    }
    return true;
}

void Print_pump_value(CLI_service& cli, uint8_t pump_index, std::optional<float> value){
    if(value.has_value()){
        cli.Print_ln(emio::format("Pump {}: {}",pump_index, value.value()));
    }else{
        cli.Print_error("failed to retrieve value");
    }
}

void Pump_module::Setup_cli(CLI_service& cli) const {
    cli.Bind("pump_stop",[this,&cli](std::vector<std::string> args)->void{
        uint8_t pump_count = pump_controller->Pump_count();
        
        if(not CLI_service::Check_argument_count(args, cli, 0, pump_count)){
            return;
        }
        
        if(args.size() == 0){
            pump_controller->Stop_all();
            cli.Print_ln("all pumps stopped");
        }
        
        for (const auto& arg : args){
            uint8_t pump_index = 0;
            if(not Parse_pump_index(arg, pump_count, cli, pump_index)){
                continue;
            }
            
            if(not pump_controller->Stop(pump_index)){
                cli.Print_error("Stop failed");
                continue;
            }
            
            cli.Print_ln("success");
        }
    },"stops all or selected pumps","[pump_index pump_index ...]?");
    
    cli.Bind("pump_count",[this,&cli]()->void{
        uint8_t pump_count = pump_controller->Pump_count();
        
        cli.Print_ln(std::to_string(pump_count));
    },"get the number of installed pumps");
    
    cli.Bind("pump_get_speed",[this,&cli](std::vector<std::string> args)->void{
        uint8_t pump_count = pump_controller->Pump_count();
        
        if(not CLI_service::Check_argument_count(args, cli, 0, pump_count)){
            return;
        }
        
        std::vector<uint8_t> selected_indexes;
        
        if(not Parse_pump_indexes(args, pump_count, cli, true, selected_indexes)){
            return;
        }
        
        for(const auto& pump_index : selected_indexes){
            std::optional<float> speed = pump_controller->Get_speed(pump_index);
            
            Print_pump_value(cli,pump_index,speed);
        }
    },"get pump speed for all, or selected pumps","[pump_index pump_index ...]?");
    
    cli.Bind("pump_get_flowrate",[this,&cli](std::vector<std::string> args)->void{
        uint8_t pump_count = pump_controller->Pump_count();
        
        if(not CLI_service::Check_argument_count(args, cli, 0, pump_count)){
            return;
        }
        
        std::vector<uint8_t> selected_indexes;
        
        if(not Parse_pump_indexes(args, pump_count, cli, true, selected_indexes)){
            return;
        }
        
        for(const auto& pump_index : selected_indexes){
            std::optional<float> flowrate = pump_controller->Get_flowrate(pump_index);
            
            Print_pump_value(cli,pump_index,flowrate);
        }
    },"get flowrate for all, or selected pumps","[pump_index pump_index ...]?");
    
    cli.Bind("pump_get_max_flowrate",[this,&cli](std::vector<std::string> args)->void{
        uint8_t pump_count = pump_controller->Pump_count();
        
        if(not CLI_service::Check_argument_count(args, cli, 0, pump_count)){
            return;
        }
        
        std::vector<uint8_t> selected_indexes;
        
        if(not Parse_pump_indexes(args, pump_count, cli, true, selected_indexes)){
            return;
        }
        
        for(const auto& pump_index : selected_indexes){
            std::optional<float> flowrate = pump_controller->Max_flowrate(pump_index);
            
            Print_pump_value(cli,pump_index,flowrate);
        }
    },"get maximal flowrate for all, or selected pumps","[pump_index pump_index ...]?");
    
    cli.Bind("pump_get_min_flowrate",[this,&cli](std::vector<std::string> args)->void{
        uint8_t pump_count = pump_controller->Pump_count();
        
        if(not CLI_service::Check_argument_count(args, cli, 0, pump_count)){
            return;
        }
        
        std::vector<uint8_t> selected_indexes;
        
        if(not Parse_pump_indexes(args, pump_count, cli, true, selected_indexes)){
            return;
        }
        
        for(const auto& pump_index : selected_indexes){
            std::optional<float> flowrate = pump_controller->Min_flowrate(pump_index);
            
            Print_pump_value(cli,pump_index,flowrate);
        }
    },"get minimal flowrate for all, or selected pumps","[pump_index pump_index ...]?");
    
    cli.Bind("pump_set_speed",[this,&cli](std::vector<std::string> args)->void{
        uint8_t pump_count = pump_controller->Pump_count();
        
        if(not CLI_service::Check_argument_count(args, cli, 2, pump_count+1)){
            return;
        }
        
        float speed = 0.0;
        if(not CLI_service::Parse_argument(args[0],cli,speed)){
            return;
        }
        
        std::vector<uint8_t> selected_indexes;
        if(not Parse_pump_indexes(args, pump_count, cli, false, selected_indexes, 1)){
            return;
        }
        
        for(const auto& pump_index : selected_indexes){
            if(not pump_controller->Set_speed(pump_index, speed)){
                cli.Print_error("Set_speed failed");
            }else{
                cli.Print_ln("success");
            }
        }
    },"set speed for selected pumps","speed(float) pump_index pump_index ...");
    
    cli.Bind("pump_set_flowrate",[this,&cli](std::vector<std::string> args)->void{
        uint8_t pump_count = pump_controller->Pump_count();
        
        if(not CLI_service::Check_argument_count(args, cli, 2, pump_count+1)){
            return;
        }
        
        float flowrate = 0.0;
        if(not CLI_service::Parse_argument(args[0],cli,flowrate)){
            return;
        }
        
        std::vector<uint8_t> selected_indexes;
        if(not Parse_pump_indexes(args, pump_count, cli, false, selected_indexes, 1)){
            return;
        }
        
        for(const auto& pump_index : selected_indexes){
            if(not pump_controller->Set_flowrate(pump_index, flowrate)){
                cli.Print_error("Set_flowrate failed");
            }else{
                cli.Print_ln("success");
            }
        }
    },"set flowrate for selected pumps","flowrate(float) pump_index pump_index ...");
    
    cli.Bind("pump_set_max_flowrate",[this,&cli](std::vector<std::string> args)->void{
        uint8_t pump_count = pump_controller->Pump_count();
        
        if(not CLI_service::Check_argument_count(args, cli, 2, pump_count+1)){
            return;
        }
        
        float flowrate = 0.0;
        if(not CLI_service::Parse_argument(args[0],cli,flowrate)){
            return;
        }
        
        std::vector<uint8_t> selected_indexes;
        if(not Parse_pump_indexes(args, pump_count, cli, false, selected_indexes, 1)){
            return;
        }
        
        for(const auto& pump_index : selected_indexes){
            if(not pump_controller->Set_max_flowrate(pump_index, flowrate)){
                cli.Print_error("Set_max_flowrate failed");
            }else{
                cli.Print_ln("success");
            }
        }
    },"set max flowrate for selected pumps", "flowrate(float) pump_index pump_index ...");
}
