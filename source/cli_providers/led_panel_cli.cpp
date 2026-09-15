#include "cli_providers/led_panel_cli.hpp"
#include "components/led_panel.hpp"

LED_panel_cli::LED_panel_cli(LED_panel* led_panel) : 
    CLI_provider(),
    led_panel(led_panel)
{
    
}

LED_panel_cli::LED_panel_cli(LED_panel* led_panel, Base_module_cli* base_module_cli) : 
    CLI_provider(base_module_cli),
    led_panel(led_panel)
{
    if(led_panel){
        Register_temperature_readout("led_panel", [this]()->std::optional<float>{
            return std::optional<float>{this->led_panel->Temperature()};
        });
    }
}

bool LED_panel_cli::Parse_channel(const std::string& arg, CLI_service& cli, uint8_t& selected_channel){
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

bool LED_panel_cli::Parse_channels(
    const std::vector<std::string>& args,
    CLI_service& cli, 
    bool fill_on_empty, 
    std::vector<uint8_t>& selected_channels,
    size_t skip_args
){    
    
    const size_t channel_count = 4;
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


void LED_panel_cli::Connect_to_cli(CLI_service& cli) const{
    if( led_panel ){
        cli.Bind("led_set_intensity",[this, &cli](std::vector<std::string> args){
            if(not cli.Check_argument_count(args,2)){
                return;
            }
            
            float intensity = 0.0;
            if(not cli.Parse_argument(args[0],intensity)){
                return;
            }
            
            
            std::vector<uint8_t> channels;
            if(not Parse_channels(args,cli,false,channels,1)){
                return;
            }
            
            for(const auto& channel : channels){
                if(this->led_panel->Set_intensity(channel, intensity)){
                    cli.Print_notice("success");
                }else{
                    cli.Print_error("Set_intensity failed");
                }
            }
        },"set intensity of selected channels","intensity(float) channel channel...");
        
        cli.Bind("led_get_intensity",[this, &cli](std::vector<std::string> args){
            if(not cli.Check_argument_count(args,0)){
                return;
            }
            
            std::vector<uint8_t> channels;
            if(not Parse_channels(args,cli,true,channels)){
                return;
            }
            
            for(const auto& channel : channels){
                std::optional<float> intensity = this->led_panel->Get_intensity(channel);
                
                if(intensity.has_value()){
                    cli.Print_ln(emio::format("channel {}: {}",channel, intensity.value()));
                }else{
                    cli.Print_error("Get_intensity failed");
                }
            }
        },"set intensity of selected channels","channel channel...");
        
        /*
        cli.Bind("led_power_limited",[led_panel, &cli](){
            if(led_panel->Power_limited()){
                cli.Print_ln("not limited");
            }else{
                cli.Print_ln(dye::yellow("limited"));
            }
        },"Detect if power of LED illumination is limited by power budget");
        
        cli.Bind("led_temp_limited",[led_panel, &cli](){
            if(led_panel->Temperature_limited()){
                cli.Print_ln("not limited");
            }else{
                cli.Print_ln(dye::yellow("limited"));
            }
        },"Detect if power of LED illumination is limited by temperature of module");
        */

        Logger::Debug("LED panel connected to cli");
    }
}