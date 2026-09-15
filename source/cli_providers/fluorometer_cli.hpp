#pragma once
#include "cli_providers/cli_provider.hpp"

class Fluorometer;

class Fluorometer_cli : public CLI_provider{
    private: 
        Fluorometer * fluorometer;
        
    public: 
        Fluorometer_cli(Fluorometer* fluorometer);
        Fluorometer_cli(Fluorometer* fluorometer, Base_module_cli* base_module_cli);

        void Connect_to_cli(CLI_service& cli) const override;
};