#pragma once
#include "cli_providers/cli_provider.hpp"

class LED_panel;

class LED_panel_cli : public CLI_provider{
    private: 
        LED_panel * led_panel;
        
    public: 
        
        /**
         * @brief Construct a new LED_panel_cli object
         *
         * @param led_panel   Pointer to the LED panel component
         */
        LED_panel_cli(LED_panel* led_panel);

        /**
         * @brief Construct a new LED_panel_cli object and register its temperature readout
         *
         * @param led_panel            Pointer to the LED panel component
         * @param base_module_cli      Pointer to the base module cli for temperature readout registration
         */
        LED_panel_cli(LED_panel* led_panel, Base_module_cli* base_module_cli);

        /**
         * @brief Binds all LED panel commands to the given cli
         *
         * @param cli   CLI_service to bind the commands to
         */
        void Connect_to_cli(CLI_service& cli) const override;

    private:
        /**
         * @brief Parses the given argument into a single LED panel channel index
         *
         * @param arg                 Argument to parse (single digit)
         * @param cli                 CLI_service used to report parse errors
         * @param selected_channel    Output channel index filled with the parsed value
         * @return true               If the channel was parsed successfully
         * @return false              If a parse error occurred
         */
        static bool Parse_channel(const std::string& arg, CLI_service& cli, uint8_t& selected_channel);

        /**
         * @brief Parses the given arguments into a list of selected LED panel channel indexes
         *
         * @param args                 Arguments to parse
         * @param cli                  CLI_service used to report parse errors
         * @param fill_on_empty        If true and no channels are given, all channels are selected
         * @param selected_channels    Output vector filled with the parsed channel indexes
         * @param skip_args            Number of leading arguments to skip before parsing channels
         * @return true                If the channels were parsed successfully
         * @return false               If a parse error occurred
         */
        static bool Parse_channels(
            const std::vector<std::string>& args,
            CLI_service& cli, 
            bool fill_on_empty, 
            std::vector<uint8_t>& selected_channels,
            size_t skip_args = 0
        );
};