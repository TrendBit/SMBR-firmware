#pragma once
#include "cli_providers/cli_provider.hpp"

class Pump_controller;

class Pump_controller_cli : public CLI_provider{
    private: 
        Pump_controller * pump_controller;

    public: 
        /**
         * @brief Construct a new Pump_controller_cli object
         *
         * @param pump_controller   Pointer to the pump controller component
         */
        Pump_controller_cli(Pump_controller* pump_controller);

        /**
         * @brief Construct a new Pump_controller_cli object
         *
         * @param pump_controller     Pointer to the pump controller component
         * @param base_module_cli     Pointer to the base module cli for temperature readout registration
         */
        Pump_controller_cli(Pump_controller* pump_controller, Base_module_cli* base_module_cli);

        /**
         * @brief Binds all pump controller commands to the given cli
         *
         * @param cli   CLI_service to bind the commands to
         */
        void Connect_to_cli(CLI_service& cli) const override;

    private: 
        /**
         * @brief Parses the given argument into a single pump index
         *
         * @param arg                  Argument to parse (single digit)
         * @param pump_count           Total number of installed pumps, used for range checking
         * @param cli                  CLI_service used to report parse errors
         * @param selected_pump_index  Output pump index filled with the parsed value
         * @return true                If the pump index was parsed successfully
         * @return false               If a parse error occurred
         */
        static bool Parse_pump_index(const std::string& arg, uint8_t pump_count, CLI_service& cli, uint8_t& selected_pump_index);

        /**
         * @brief Parses the given arguments into a list of selected pump indexes
         *
         * @param args                  Arguments to parse
         * @param pump_count            Total number of installed pumps, used for range checking
         * @param cli                   CLI_service used to report parse errors
         * @param fill_on_empty         If true and no indexes are given, all pumps are selected
         * @param selected_pump_indexes Output vector filled with the parsed pump indexes
         * @param skip_args             Number of leading arguments to skip before parsing indexes
         * @return true                 If the pump indexes were parsed successfully
         * @return false                If a parse error occurred
         */
        static bool Parse_pump_indexes(
            const std::vector<std::string>& args,
            uint8_t pump_count, 
            CLI_service& cli, 
            bool fill_on_empty, 
            std::vector<uint8_t>& selected_pump_indexes,
            size_t skip_args = 0
        );

        /**
         * @brief Prints the value of a pump readout or an error if the value is not available
         *
         * @param cli            CLI_service used for printing
         * @param pump_index     Index of the pump the value belongs to
         * @param value          Optional value to print, empty if retrieval failed
         */
        static void Print_pump_value(CLI_service& cli, uint8_t pump_index, std::optional<float> value);
};