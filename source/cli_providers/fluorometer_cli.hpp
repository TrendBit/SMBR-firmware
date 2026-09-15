#pragma once
#include "cli_providers/cli_provider.hpp"

class Fluorometer;

class Fluorometer_cli : public CLI_provider{
    private: 
        Fluorometer * fluorometer;
        
    public: 
        /**
         * @brief Construct a new Fluorometer_cli object
         *
         * @param fluorometer   Pointer to the fluorometer component
         */
        Fluorometer_cli(Fluorometer* fluorometer);

        /**
         * @brief Construct a new Fluorometer_cli object and register its temperature readouts
         *
         * @param fluorometer          Pointer to the fluorometer component
         * @param base_module_cli      Pointer to the base module cli for temperature readout registration
         */
        Fluorometer_cli(Fluorometer* fluorometer, Base_module_cli* base_module_cli);

        /**
         * @brief Binds all fluorometer commands to the given cli
         *
         * @param cli   CLI_service to bind the commands to
         */
        void Connect_to_cli(CLI_service& cli) const override;
};