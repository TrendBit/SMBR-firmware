#include "cli_providers/bottle_temperature_cli.hpp"
#include "components/bottle_temperature.hpp"

Bottle_temperature_cli::Bottle_temperature_cli(Bottle_temperature * bottle_temperature):
    CLI_provider(),
    bottle_temperature(bottle_temperature)
{
    
}


Bottle_temperature_cli::Bottle_temperature_cli(Bottle_temperature * bottle_temperature, Base_module_cli* base_module_cli):
    CLI_provider(base_module_cli),
    bottle_temperature(bottle_temperature)
{
    if (bottle_temperature) {
        Register_temperature_readout("bottle", [this]()->std::optional<float>{
            return std::optional<float>{this->bottle_temperature->Temperature()};
        });
        
        Register_temperature_readout("bottle_top", [this]()->std::optional<float>{
            return std::optional<float>{this->bottle_temperature->Top_temperature()};
        });
        
        Register_temperature_readout("bottle_sensor_top", [this]()->std::optional<float>{
            return std::optional<float>{this->bottle_temperature->Top_sensor_temperature()};
        });
        
        Register_temperature_readout("bottle_bottom", [this]()->std::optional<float>{
            return std::optional<float>{this->bottle_temperature->Bottom_temperature()};
        });
        
        Register_temperature_readout("bottle_sensor_bottom", [this]()->std::optional<float>{
            return std::optional<float>{this->bottle_temperature->Bottom_sensor_temperature()};
        });
    }
}
