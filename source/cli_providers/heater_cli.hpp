#pragma once
#include "cli_providers/cli_provider.hpp"

class Heater;

class Heater_cli : public CLI_provider{
    private: 
        Heater * heater;

    public:  
        /**
         * @brief Construct a new Heater_cli object
         *
         * @param heater   Pointer to the heater component
         */
        Heater_cli(Heater* heater);

        /**
         * @brief Construct a new Heater_cli object and register its temperature readout
         *
         * @param heater               Pointer to the heater component
         * @param base_module_cli      Pointer to the base module cli for temperature readout registration
         */
        Heater_cli(Heater* heater, Base_module_cli* base_module_cli);

};

