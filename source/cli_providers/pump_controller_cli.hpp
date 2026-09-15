#pragma once
#include "cli_providers/cli_provider.hpp"

class Pump_controller;

class Pump_controller_cli : public CLI_provider{
    private: 
        Pump_controller * pump_controller;

    public: 
        Pump_controller_cli(Pump_controller* pump_controller);
        Pump_controller_cli(Pump_controller* pump_controller, Base_module_cli* base_module_cli);

        void Connect_to_cli(CLI_service& cli) const override;

    private: 
        static bool Parse_pump_index(const std::string& arg, uint8_t pump_count, CLI_service& cli, uint8_t& selected_pump_index);
        static bool Parse_pump_indexes(
            const std::vector<std::string>& args,
            uint8_t pump_count, 
            CLI_service& cli, 
            bool fill_on_empty, 
            std::vector<uint8_t>& selected_pump_indexes,
            size_t skip_args = 0
        );
        static void Print_pump_value(CLI_service& cli, uint8_t pump_index, std::optional<float> value);
};