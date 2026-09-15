#pragma once
#include "cli.hpp"

#include <functional>
#include <optional>
#include <string>

class Base_module_cli;

class CLI_provider {
    private:
        Base_module_cli* base_module_cli;
        
    public:
        /**
         * @brief Construct a new CLI_provider object without a base module cli
         */
        CLI_provider();

        /**
         * @brief Construct a new CLI_provider object bound to the given base module cli
         *
         * @param base_module_cli   Pointer to the base module cli used for temperature readout registration
         */
        CLI_provider(Base_module_cli* base_module_cli);

        /**
         * @brief Method implemented by derived class, which should bind all of its commands to the given cli
         */
        virtual void Connect_to_cli(CLI_service& cli) const;
    
    protected:
        /**
         * @brief Register the given getter_function as a temperature readout (generaly from a sensor) under the given 
         *        name. It is later used to provide a structured readout of all installed sensors.
         * 
         * @param readout_name      Name under which the readout will be refered to.
         * @param getter_function   A function that provides the readouts current value.
         */
        void Register_temperature_readout(std::string readout_name, std::function<std::optional<float>()> getter_function);
};