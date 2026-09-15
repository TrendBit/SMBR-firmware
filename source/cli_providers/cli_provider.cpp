#include "cli_providers/cli_provider.hpp"
#include "cli_providers/base_module_cli.hpp"

CLI_provider::CLI_provider():
    base_module_cli(nullptr)
{
    
}

CLI_provider::CLI_provider(Base_module_cli* base_module_cli):
    base_module_cli(base_module_cli)
{
    
}
    
void CLI_provider::Register_temperature_readout(std::string readout_name, std::function<std::optional<float>()> getter_function){ 
    if(base_module_cli){
        base_module_cli->Register_temperature_readout(readout_name,std::move(getter_function));
    }
}

void CLI_provider::Connect_to_cli(CLI_service& cli) const {
    
}