#pragma once
#include "cli_providers/cli_provider.hpp"

class Bottle_temperature;

class Bottle_temperature_cli : public CLI_provider{
    private: 
        Bottle_temperature * bottle_temperature;
        
    public: 
        /**
         * @brief Construct a new Bottle_temperature_cli object
         *
         * @param bottle_temperature   Pointer to the bottle temperature component
         */
        Bottle_temperature_cli(Bottle_temperature * bottle_temperature);

        /**
         * @brief Construct a new Bottle_temperature_cli object and register its temperature readouts
         *
         * @param bottle_temperature   Pointer to the bottle temperature component
         * @param base_module_cli      Pointer to the base module cli for temperature readout registration
         */
        Bottle_temperature_cli(Bottle_temperature * bottle_temperature, Base_module_cli* base_module_cli);
};