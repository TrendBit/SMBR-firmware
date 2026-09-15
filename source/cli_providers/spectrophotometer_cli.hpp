#pragma once
#include "cli_providers/cli_provider.hpp"
#include "components/spectrophotometer.hpp"

class Spectrophotometer;

class Spectrophotometer_cli : public CLI_provider{
    private: 
        Spectrophotometer * spectrophotometer;
    public: 
        
        Spectrophotometer_cli(Spectrophotometer * spectrophotometer);
        Spectrophotometer_cli(Spectrophotometer * spectrophotometer, Base_module_cli* base_module_cli);

        void Connect_to_cli(CLI_service& cli) const override;
    private:
        static bool Parse_channels(
            const std::vector<std::string>& args,
            CLI_service& cli, 
            bool fill_on_empty, 
            std::vector<Spectrophotometer::Channels>& selected_channels,
            size_t skip_args = 0
        );
};