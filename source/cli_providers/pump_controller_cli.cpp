#include "cli_providers/pump_controller_cli.hpp"
#include "components/pumps.hpp"

Pump_controller_cli::Pump_controller_cli(Pump_controller* pump_controller) :
    CLI_provider(),
    pump_controller(pump_controller)
{
    
}

Pump_controller_cli::Pump_controller_cli(Pump_controller* pump_controller, Base_module_cli* base_module_cli) :
    CLI_provider(base_module_cli),
    pump_controller(pump_controller)
{
    
}


bool Pump_controller_cli::Parse_pump_index(const std::string& arg, uint8_t pump_count, CLI_service& cli, uint8_t& selected_pump_index){
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

bool Pump_controller_cli::Parse_pump_indexes(
    const std::vector<std::string>& args,
    uint8_t pump_count, 
    CLI_service& cli, 
    bool fill_on_empty, 
    std::vector<uint8_t>& selected_pump_indexes,
    size_t skip_args
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

void Pump_controller_cli::Print_pump_value(CLI_service& cli, uint8_t pump_index, std::optional<float> value){
    if(value.has_value()){
        cli.Print_ln(emio::format("Pump {}: {}",pump_index, value.value()));
    }else{
        cli.Print_error("failed to retrieve value");
    }
}

void Pump_controller_cli::Connect_to_cli(CLI_service& cli) const{
    if(pump_controller){
        cli.Bind("pump_stop",[this,&cli](std::vector<std::string> args)->void{
            uint8_t pump_count = this->pump_controller->Pump_count();
            
            if(not cli.Check_argument_count(args, 0, pump_count)){
                return;
            }
            
            if(args.size() == 0){
                this->pump_controller->Stop_all();
                cli.Print_notice("all pumps stopped");
            }
            
            for (const auto& arg : args){
                uint8_t pump_index = 0;
                if(not Parse_pump_index(arg, pump_count, cli, pump_index)){
                    continue;
                }
                
                if(not this->pump_controller->Stop(pump_index)){
                    cli.Print_error("Stop failed");
                    continue;
                }
                
                cli.Print_notice("success");
            }
        },"stops all or selected pumps","[pump_index pump_index ...]?");
        
        cli.Bind("pump_count",[this,&cli]()->void{
            uint8_t pump_count = this->pump_controller->Pump_count();
            
            cli.Print_ln(std::to_string(pump_count));
        },"get the number of installed pumps");
        
        cli.Bind("pump_get_speed",[this,&cli](std::vector<std::string> args)->void{
            uint8_t pump_count = this->pump_controller->Pump_count();
            
            if(not cli.Check_argument_count(args, 0, pump_count)){
                return;
            }
            
            std::vector<uint8_t> selected_indexes;
            
            if(not Parse_pump_indexes(args, pump_count, cli, true, selected_indexes)){
                return;
            }
            
            for(const auto& pump_index : selected_indexes){
                std::optional<float> speed = this->pump_controller->Get_speed(pump_index);
                
                Print_pump_value(cli,pump_index,speed);
            }
        },"get pump speed for all, or selected pumps","[pump_index pump_index ...]?");
        
        cli.Bind("pump_get_flowrate",[this,&cli](std::vector<std::string> args)->void{
            uint8_t pump_count = this->pump_controller->Pump_count();
            
            if(not cli.Check_argument_count(args, 0, pump_count)){
                return;
            }
            
            std::vector<uint8_t> selected_indexes;
            
            if(not Parse_pump_indexes(args, pump_count, cli, true, selected_indexes)){
                return;
            }
            
            for(const auto& pump_index : selected_indexes){
                std::optional<float> flowrate = this->pump_controller->Get_flowrate(pump_index);
                
                Print_pump_value(cli,pump_index,flowrate);
            }
        },"get flowrate for all, or selected pumps","[pump_index pump_index ...]?");
        
        cli.Bind("pump_get_max_flowrate",[this,&cli](std::vector<std::string> args)->void{
            uint8_t pump_count = this->pump_controller->Pump_count();
            
            if(not cli.Check_argument_count(args, 0, pump_count)){
                return;
            }
            
            std::vector<uint8_t> selected_indexes;
            
            if(not Parse_pump_indexes(args, pump_count, cli, true, selected_indexes)){
                return;
            }
            
            for(const auto& pump_index : selected_indexes){
                std::optional<float> flowrate = this->pump_controller->Max_flowrate(pump_index);
                
                Print_pump_value(cli,pump_index,flowrate);
            }
        },"get maximal flowrate for all, or selected pumps","[pump_index pump_index ...]?");
        
        cli.Bind("pump_get_min_flowrate",[this,&cli](std::vector<std::string> args)->void{
            uint8_t pump_count = this->pump_controller->Pump_count();
            
            if(not cli.Check_argument_count(args, 0, pump_count)){
                return;
            }
            
            std::vector<uint8_t> selected_indexes;
            
            if(not Parse_pump_indexes(args, pump_count, cli, true, selected_indexes)){
                return;
            }
            
            for(const auto& pump_index : selected_indexes){
                std::optional<float> flowrate = this->pump_controller->Min_flowrate(pump_index);
                
                Print_pump_value(cli,pump_index,flowrate);
            }
        },"get minimal flowrate for all, or selected pumps","[pump_index pump_index ...]?");
        
        cli.Bind("pump_set_speed",[this,&cli](std::vector<std::string> args)->void{
            uint8_t pump_count = this->pump_controller->Pump_count();
            
            if(not cli.Check_argument_count(args, 2, pump_count+1)){
                return;
            }
            
            float speed = 0.0;
            if(not cli.Parse_argument(args[0],speed)){
                return;
            }
            
            std::vector<uint8_t> selected_indexes;
            if(not Parse_pump_indexes(args, pump_count, cli, false, selected_indexes, 1)){
                return;
            }
            
            for(const auto& pump_index : selected_indexes){
                if(not this->pump_controller->Set_speed(pump_index, speed)){
                    cli.Print_error("Set_speed failed");
                }else{
                    cli.Print_notice("success");
                }
            }
        },"set speed for selected pumps","speed(float) pump_index pump_index ...");
        
        cli.Bind("pump_set_flowrate",[this,&cli](std::vector<std::string> args)->void{
            uint8_t pump_count = this->pump_controller->Pump_count();
            
            if(not cli.Check_argument_count(args, 2, pump_count+1)){
                return;
            }
            
            float flowrate = 0.0;
            if(not cli.Parse_argument(args[0],flowrate)){
                return;
            }
            
            std::vector<uint8_t> selected_indexes;
            if(not Parse_pump_indexes(args, pump_count, cli, false, selected_indexes, 1)){
                return;
            }
            
            for(const auto& pump_index : selected_indexes){
                if(not this->pump_controller->Set_flowrate(pump_index, flowrate)){
                    cli.Print_error("Set_flowrate failed");
                }else{
                    cli.Print_notice("success");
                }
            }
        },"set flowrate for selected pumps","flowrate(float) pump_index pump_index ...");
        
        cli.Bind("pump_set_max_flowrate",[this,&cli](std::vector<std::string> args)->void{
            uint8_t pump_count = this->pump_controller->Pump_count();
            
            if(not cli.Check_argument_count(args, 2, pump_count+1)){
                return;
            }
            
            float flowrate = 0.0;
            if(not cli.Parse_argument(args[0],flowrate)){
                return;
            }
            
            std::vector<uint8_t> selected_indexes;
            if(not Parse_pump_indexes(args, pump_count, cli, false, selected_indexes, 1)){
                return;
            }
            
            for(const auto& pump_index : selected_indexes){
                if(not this->pump_controller->Set_max_flowrate(pump_index, flowrate)){
                    cli.Print_error("Set_max_flowrate failed");
                }else{
                    cli.Print_notice("success");
                }
            }
        },"set max flowrate for selected pumps", "flowrate(float) pump_index pump_index ...");

        Logger::Debug("pump controller connected to cli");
    }
}