#include "cli_providers/spectrophotometer_cli.hpp"

Spectrophotometer_cli::Spectrophotometer_cli(Spectrophotometer * spectrophotometer):
    CLI_provider(),
    spectrophotometer(spectrophotometer)
{
    
}
Spectrophotometer_cli::Spectrophotometer_cli(Spectrophotometer * spectrophotometer, Base_module_cli* base_module_cli):
    CLI_provider(base_module_cli),
    spectrophotometer(spectrophotometer)
{
    if(spectrophotometer){
        Register_temperature_readout("spectrophotometer", [this]()->std::optional<float>{
            return std::optional<float>{this->spectrophotometer->Temperature()};
        });
    }
}

bool Spectrophotometer_cli::Parse_channels(
    const std::vector<std::string>& args,
    CLI_service& cli, 
    bool fill_on_empty, 
    std::vector<Spectrophotometer::Channels>& selected_channels,
    size_t skip_args
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

void Spectrophotometer_cli::Connect_to_cli(CLI_service& cli) const {
    if(spectrophotometer){
        cli.Bind("spectrophotometer_measure",[this, &cli](std::vector<std::string> args){
            if(not cli.Check_argument_count(args, 0)){
                return;
            }
            
            std::vector<Spectrophotometer::Channels> channels;
            if (not Parse_channels(args, cli, true, channels)){
                return;
            }
            
            cli.Print_notice("CHANNEL  ABSOLUTE  RELATIVE");
            
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
            if(not cli.Check_argument_count(args, 0)){
                return;
            }
            
            std::vector<Spectrophotometer::Channels> channels;
            if (not Parse_channels(args, cli, true, channels)){
                return;
            }
            
            cli.Print_notice("CHANNEL  INTENSITY");
            
            for(const auto& channel : channels){
                auto measurement = spectrophotometer->Measure_intensity(channel);
                cli.Print_ln(
                    emio::format("{}: {}%",
                        magic_enum::enum_name(channel),
                        measurement
                ));
            }
        },"measure all, or selected channels intensity","[channel channel ...]?");

        Logger::Debug("spectrophotometer connected to cli");
    }
}