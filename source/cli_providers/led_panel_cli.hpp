#pragma once
#include "cli_providers/cli_provider.hpp"

class LED_panel;

class LED_panel_cli : public CLI_provider{
    private: 
        LED_panel * led_panel;
        
    public: 
        
        LED_panel_cli(LED_panel* led_panel);
        LED_panel_cli(LED_panel* led_panel, Base_module_cli* base_module_cli);

        void Connect_to_cli(CLI_service& cli) const override;

    private:
        static bool Parse_channel(const std::string& arg, CLI_service& cli, uint8_t& selected_channel);
        static bool Parse_channels(
            const std::vector<std::string>& args,
            CLI_service& cli, 
            bool fill_on_empty, 
            std::vector<uint8_t>& selected_channels,
            size_t skip_args = 0
        );
};