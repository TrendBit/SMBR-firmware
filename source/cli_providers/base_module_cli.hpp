#pragma once
#include "cli_providers/cli_provider.hpp"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

class Base_module;

class Base_module_cli : public CLI_provider {
    friend class CLI_provider;
private:
    Base_module* base_module;

    /**
        * @brief   Holds registered temperature readouts
        */
    std::unordered_map<std::string, std::function<std::optional<float>()>> temperature_readouts;
    
public:
    /**
     * @brief Construct a new Base_module_cli object
     *
     * @param base_module   Pointer to the base module providing temperature readouts
     */
    Base_module_cli(Base_module* base_module);

    /**
     * @brief Binds all base module commands (module info, temperatures, instance) to the given cli
     *
     * @param cli   CLI_service to bind the commands to
     */
    void Connect_to_cli(CLI_service& cli) const override;
    
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