#pragma once
#include "cli_providers/cli_provider.hpp"

class Bottle_temperature;

class Bottle_temperature_cli : public CLI_provider{
    private: 
        Bottle_temperature * bottle_temperature;
        
    public: 
        Bottle_temperature_cli(Bottle_temperature * bottle_temperature);
        Bottle_temperature_cli(Bottle_temperature * bottle_temperature, Base_module_cli* base_module_cli);
};