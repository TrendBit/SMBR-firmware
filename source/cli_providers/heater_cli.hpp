#pragma once
#include "cli_providers/cli_provider.hpp"

class Heater;

class Heater_cli : public CLI_provider{
    private: 
        Heater * heater;

    public:  
        Heater_cli(Heater* heater);
        Heater_cli(Heater* heater, Base_module_cli* base_module_cli);

};

