#include "cli_providers/fluorometer_cli.hpp"
#include "components/fluorometer.hpp"

Fluorometer_cli::Fluorometer_cli(Fluorometer* fluorometer):
    CLI_provider(),
    fluorometer(fluorometer)
{
    
}

Fluorometer_cli::Fluorometer_cli(Fluorometer* fluorometer, Base_module_cli* base_module_cli):
    CLI_provider(base_module_cli),
    fluorometer(fluorometer)
{
    if(fluorometer){
        Register_temperature_readout("fluorometer_emitor", [this]()->std::optional<float>{
            return std::optional<float>{this->fluorometer->Emitor_temperature()};
        });
        Register_temperature_readout("fluorometer_detector", [this]()->std::optional<float>{
            return std::optional<float>{this->fluorometer->Detector_temperature()};
        });
    }
}

void Fluorometer_cli::Connect_to_cli(CLI_service& cli) const {
    if(fluorometer){
        cli.Bind("fluorometer_capture",[this, &cli](std::vector<std::string> args){
            if( not cli.Check_argument_count(args, 3, 5)){
                return;
            }
            
            Fluorometer_config::Gain gain;
            const auto& gain_arg = args[0];
            
            if(gain_arg == "x1"){
                gain = Fluorometer_config::Gain::x1;
            }else if(gain_arg == "x10"){
                gain = Fluorometer_config::Gain::x10;
            }else if(gain_arg == "x50"){
                gain = Fluorometer_config::Gain::x50;
            }else if(gain_arg == "Auto"){
                gain = Fluorometer_config::Gain::Auto;
            }else{
                cli.Print_error("Invalid gain selected");
                return;
            }
            
            float intensity = 0.0;
            float capture_length = 0.0;
            if( not cli.Parse_argument(args[1],intensity)
            ||  not cli.Parse_argument(args[2],capture_length)
            ){
                return;
            }
            
            if(intensity < 0.1 || intensity > 1.0) {
                cli.Print_error("Intensity out of range");
                return;
            }
            
            // samples defined
            if(args.size() == 4){
                uint samples = 0;
                if( not cli.Parse_argument(args[3],samples)){
                    return;
                }
                
                if(this->fluorometer->Capture_OJIP(gain, intensity, capture_length, samples)){
                    cli.Print_notice("success");
                }else{
                    cli.Print_error("Capture_OJIP failed");
                }
                
                return;
            }
            
            // samples and timing defined
            if(args.size() == 5){
                uint samples = 0;
                if( not cli.Parse_argument(args[3],samples)){
                    return;
                }
                Fluorometer_config::Timing timing;
                const auto& timing_arg = args[4];
                
                if(timing_arg == "Linear"){
                    timing = Fluorometer_config::Timing::Linear;
                }else if(timing_arg == "Logarithmic"){
                    timing = Fluorometer_config::Timing::Logarithmic;
                }else{
                    cli.Print_error("Invalid timing selected");
                    return;
                }
                
                if(this->fluorometer->Capture_OJIP(gain, intensity, capture_length, samples, timing)){
                    cli.Print_notice("success");
                }else{
                    cli.Print_error("Capture_OJIP failed");
                }
                
                return;
            }
            
            if(this->fluorometer->Capture_OJIP(gain, intensity, capture_length)){
                cli.Print_notice("success");
            }else{
                cli.Print_error("Capture_OJIP failed");
            }
            
        },"create an OJIP capture","gain(x1 | x10 | x50 | Auto) emitor_intesity(0.1 - 1.0) capture_length(float) [samples(int)]? [timing(Linear | Logarithmic)]?");
        
        cli.Bind("fluorometer_check",[this, &cli](){
            if(this->fluorometer->Capture_done()){
                cli.Print_ln("done");
            }else{
                cli.Print_ln(dye::yellow("in progress"));
            }
            
        },"check if the fluorometer capture is complete");
        
        cli.Bind("fluorometer_retrieve",[this, &cli](){
            if(this->fluorometer->Capture_done()){
                const Fluorometer::OJIP& data = *this->fluorometer->Retrieve_OJIP();
                cli.Print_ln(emio::format("ID: {}",data.measurement_id));
                cli.Print_ln(emio::format("emitor intensity: {}",data.emitor_intensity));
                cli.Print_ln(emio::format("detector gain:    {}",magic_enum::enum_name(data.detector_gain)));
                cli.Print_ln(emio::format("sample range:     {}",data.sample_range));
                cli.Print_ln(emio::format("sample count:     {}",data.sample_count));
                cli.Print_ln("SAMPLES START --------------------------");
                for(size_t i = 0; i < data.sample_count; i++){
                    cli.Print_ln(emio::format("{} | {}",data.sample_time_us[i], data.intensity[i]));
                }
                cli.Print_ln("SAMPLES END --------------------------");
            }else{
                cli.Print_error("in progress");
            }
        },"retrieve the last capture data");

        cli.Bind("fluorometer_calibrate",[this, &cli](){
            if(!this->fluorometer->Capture_done()){
                cli.Print_error("capture in progress");
                return;
            }

            this->fluorometer->Calibrate();
            
        },"runs a calibration run and saves the result to EEPROM");

        cli.Bind("fluorometer_get_config",[this, &cli](){
            cli.Print("Calibrated: ");
            if(this->fluorometer->Is_calibrated()){
                cli.Print_ln("true");
            }else{
                cli.Print_ln("false");
            }
            cli.Print("Using calibration: ");
            if(this->fluorometer->Calibration()){
                cli.Print_ln("true");
            }else{
                cli.Print_ln("false");
            }
            cli.Print("Using filtering: ");
            if(this->fluorometer->Filtering()){
                cli.Print_ln("true");
            }else{
                cli.Print_ln("false");
            }
        },"get fluorometer configuration (calibration state, filtering etc.)");

        cli.Bind("fluorometer_erase_calibration",[this, &cli](){
            if(!this->fluorometer->Is_calibrated()){
                cli.Print_error("no calibration loaded");
                return;
            }
            
            if(this->fluorometer->Erase_calibration()){
                cli.Print_notice("success");
            }else{
                cli.Print_error("unable to erase calibration");
            }
        },"erase loaded calibration data");

        cli.Bind("fluorometer_set_calibration", [this, &cli](std::vector<std::string> args){
            if( not cli.Check_argument_count(args, 1, 1)){
                return;
            }

            bool new_state;
            if( not cli.Parse_argument(args[0], new_state)){
                return;
            }

            if( this->fluorometer->Calibration(new_state)){
                cli.Print_notice("success");
            }else{
                cli.Print_error("unable to set calibration");
            }
            
        },"set fluorometer calibration use","state(bool)");

        cli.Bind("fluorometer_set_filtering", [this, &cli](std::vector<std::string> args){
            if( not cli.Check_argument_count(args, 1, 1)){
                return;
            }

            bool new_state;
            if( not cli.Parse_argument(args[0], new_state)){
                return;
            }

            if( this->fluorometer->Filtering(new_state)){
                cli.Print_notice("success");
            }else{
                cli.Print_error("unable to set filtering");
            }
        },"set fluorometer filtering use","state(bool)");

        Logger::Debug("fluorometer connected to cli");
    }
}