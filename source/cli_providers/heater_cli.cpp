#include "cli_providers/heater_cli.hpp"
#include "components/heater.hpp"

Heater_cli::Heater_cli(Heater* heater) : 
    CLI_provider(),
    heater(heater)
{
    
}

Heater_cli::Heater_cli(Heater* heater, Base_module_cli* base_module_cli) : 
    CLI_provider(base_module_cli),
    heater(heater)
{
    if (heater) {
        Register_temperature_readout("heater", [this]()->std::optional<float>{
            return std::optional<float>{this->heater->Temperature()};
        });

        Logger::Debug("heater connected to cli");
    }
}