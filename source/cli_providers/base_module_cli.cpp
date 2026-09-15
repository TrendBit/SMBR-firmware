#include "cli_providers/base_module_cli.hpp"
#include "modules/base_module.hpp"

Base_module_cli::Base_module_cli(Base_module* base_module):
    CLI_provider(this),
    base_module(base_module)
{
    if(base_module){
        Register_temperature_readout("board",[this]()->std::optional<float>{
            return this->base_module->Board_temperature();
        });
    }
}


void Base_module_cli::Register_temperature_readout(std::string readout_name, std::function<std::optional<float>()> getter_function){
    this->temperature_readouts[readout_name] = std::move(getter_function);

    Logger::Trace("New temperature sensor registered to cli: {}",readout_name);
}

void Base_module_cli::Connect_to_cli(CLI_service &cli) const{
    if(base_module){
        cli.Bind("module_info",[this,&cli]()->void{
            std::string result = "";
            result += emio::format("Module type: {}\r\n", magic_enum::enum_name(this->base_module->module_type));
            result += emio::format("Instance: {}\r\n", magic_enum::enum_name(this->base_module->Instance_enumeration()));
            result += emio::format("Unique ID: ");
            for(const auto& uid_part : this->base_module->UID()){
                result += emio::format("{:x}",uid_part);
            }
            result += "\r\n";
            cli.Print(result);
        }, "Basic info about this module.");
        
        cli.Bind("temperatures", [this, &cli](std::vector<std::string> args)-> void{
            if(args.size() == 0){
                for(const auto& [name, getter] : this->temperature_readouts){
                    std::optional<float> temperature = getter();
                    if(temperature.has_value()){
                        cli.Print_ln(emio::format("{}: {}°C",name,temperature.value()));
                    }else{
                        cli.Print_ln(emio::format("{}: err",name));
                    }
                }
            }else{
                for(const auto& arg : args){
                    //returns .end() if the key isn't in the map
                    auto map_iterator = this->temperature_readouts.find(arg); 
                    
                    if(map_iterator != this->temperature_readouts.end()){
                        std::optional<float> temperature = map_iterator->second();
                        if(temperature.has_value()){
                            cli.Print_ln(emio::format("{}: {}°C",arg,temperature.value()));
                        }else{
                            cli.Print_ln(emio::format("{}: err",arg));
                        }
                    }else{
                        cli.Print_error("unknown temperature readout");
                    }
                    
                }
            }
        }, "the current temperature of all, or selected installed sensors","[sensor sensor ...]?");
    
        if(this->base_module->enumerator){
            if( this->base_module->enumerator->Instance() != Codes::Instance::Exclusive){
                cli.Bind("set_instance", [this, &cli](std::vector<std::string> args)->void{
                    if( not cli.Check_argument_count(args, 1, 1)){
                        return;
                    }
                    
                    Codes::Instance selected_instance_parsed = Codes::Instance::Undefined;
                    const auto& arg = args[0];
                    if(arg == "1"){
                        selected_instance_parsed = Codes::Instance::Instance_1;
                    }else if(arg == "2"){
                        selected_instance_parsed = Codes::Instance::Instance_2;
                    }else if(arg == "3"){
                        selected_instance_parsed = Codes::Instance::Instance_3;
                    }else if(arg == "4"){
                        selected_instance_parsed = Codes::Instance::Instance_4;
                    }else if(arg == "5"){
                        selected_instance_parsed = Codes::Instance::Instance_5;
                    }else if(arg == "6"){
                        selected_instance_parsed = Codes::Instance::Instance_6;
                    }else if(arg == "7"){
                        selected_instance_parsed = Codes::Instance::Instance_7;
                    }else if(arg == "8"){
                        selected_instance_parsed = Codes::Instance::Instance_8;
                    }else if(arg == "9"){
                        selected_instance_parsed = Codes::Instance::Instance_9;
                    }else if(arg == "10"){
                        selected_instance_parsed = Codes::Instance::Instance_10;
                    }else if(arg == "11"){
                        selected_instance_parsed = Codes::Instance::Instance_11;
                    }else if(arg == "12"){
                        selected_instance_parsed = Codes::Instance::Instance_12;
                    }else{
                        cli.Print_error("invalid instance");
                        return;
                    }
                    
                    
                    if(not this->base_module->enumerator->Enumerate(selected_instance_parsed)){
                        cli.Print_error("unable to enumerate instance");
                    }else{
                        cli.Print_notice("success");
                    }
                },"set the instance index of this module (only works for modules with instance other than Exclusive).","target_instance(1-12)");
            }
        }

        Logger::Debug("base module connected to cli");
    }
}