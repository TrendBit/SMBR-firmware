#pragma once
#include "cli_providers/cli_provider.hpp"
#include "components/spectrophotometer.hpp"

class Spectrophotometer;

class Spectrophotometer_cli : public CLI_provider{
    private: 
        Spectrophotometer * spectrophotometer;
    public: 
        
        /**
         * @brief Construct a new Spectrophotometer_cli object
         *
         * @param spectrophotometer   Pointer to the spectrophotometer component
         */
        Spectrophotometer_cli(Spectrophotometer * spectrophotometer);

        /**
         * @brief Construct a new Spectrophotometer_cli object and register its temperature readout
         *
         * @param spectrophotometer    Pointer to the spectrophotometer component
         * @param base_module_cli      Pointer to the base module cli for temperature readout registration
         */
        Spectrophotometer_cli(Spectrophotometer * spectrophotometer, Base_module_cli* base_module_cli);

        /**
         * @brief Binds all spectrophotometer commands to the given cli
         *
         * @param cli   CLI_service to bind the commands to
         */
        void Connect_to_cli(CLI_service& cli) const override;
    private:
        /**
         * @brief Parses the given arguments into a list of selected spectrophotometer channels
         *
         * @param args                 Arguments to parse
         * @param cli                  CLI_service used to report parse errors
         * @param fill_on_empty        If true and no channels are given, all channels are selected
         * @param selected_channels    Output vector filled with the parsed channels
         * @param skip_args            Number of leading arguments to skip before parsing channels
         * @return true                If the channels were parsed successfully
         * @return false               If a parse error occurred
         */
        static bool Parse_channels(
            const std::vector<std::string>& args,
            CLI_service& cli, 
            bool fill_on_empty, 
            std::vector<Spectrophotometer::Channels>& selected_channels,
            size_t skip_args = 0
        );
};