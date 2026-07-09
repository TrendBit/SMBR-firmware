#include "cli.hpp"
#include "tools/color.hpp"
#include <charconv>
#include "config.hpp"
#include "config/usb_descriptors.h"
#include "tools/cpp_proxy.hpp"

CLI_service::CLI_service():cli(new CLI(0, 256, 32,"\033[94m>\033[0m ")){

    auto status = [this]()->void {
          Status();
    };

    auto device_info_print = [this]()->void {
          cli->Print(Device_info());
    };

    Bind("status", status,"None");
    Bind("device_info", device_info_print, "Constains version, build timestamp, git commit hash, etc.");
    Bind("bootloader", [this]()->void { Bootloader(); }, "Reboots MCU into bootloader mode for fw update");
    Bind("restart", [this]()->void { Restart(); }, "Restart MCU using watchdog");
    Bind("thread_statistics", [this]()->void { Thread_statistics(); }, "Print statistics of FreeRTOS threads");
    
    Bind("command_list",[this]()->void { this->Print_command_list(); }, "Print all of the supported module commands without help messages");

    /**
     * @brief Service thread for CLI
     */
    cli_service_thread = new rtos::Lambda_thread("cli_service",[this](){
        while(1){
            cli->Service();
            rtos::Delay(10);
        }
    }, 1024, 8);
}

void CLI_service::Print_command_list(){
    for(const auto& command : this->registered_commands){
        this->Print_ln(command);
    }
}

std::string CLI_service::Device_info(){
    char unique_id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];  // Each byte will be 2 hex digits, plus null terminator
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
        sprintf(&unique_id[i*2], "%02x", id.id[i]);
    }

    std::string device_info = "";
    device_info += emio::format("Device name: {}\r\n", Generate_product_name_cpp());
    device_info += emio::format("MCU Unique ID: {}\r\n", unique_id);
    device_info += emio::format("Vendor: {}\r\n", VENDOR_NAME);
    device_info += emio::format("Build timestamp: {}\r\n", __TIMESTAMP__);
    device_info += emio::format("Firmware version: {}.{}.{}\r\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH);
    device_info += emio::format("Git commit hash: {}\r\n", FW_GIT_COMMIT_HASH_STR);
    device_info += emio::format("Compiler: {} {}.{}.{}\r\n", FW_COMPILER_NAME, __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    
    device_info += "Watchdog: ";
    #ifdef CONFIG_WATCHDOG
    device_info += "Enabled";
    #else
    device_info += "Disabled";
    #endif
    device_info += "\r\n";
    
    device_info += "Logger uart output: ";
    #ifdef CONFIG_LOGGER_UART
    device_info += "Enabled";
    #else
    device_info += "Disabled";
    #endif
    device_info += "\r\n";
    
    device_info += "Logger usb output: ";
    #ifdef CONFIG_LOGGER_USB
    device_info += "Enabled";
    #else
    device_info += "Disabled";
    #endif
    device_info += "\r\n";
    
    device_info += "Logger level: ";
    #ifdef CONFIG_LOGGER_LEVEL
    device_info += emio::format("{}",CONFIG_LOGGER_LEVEL);
    #else
    device_info += "Disabled"
    #endif
    device_info += "\r\n";
    
    device_info += "Bootloader: ";
    #ifdef CONFIG_BOOTLOADER
    device_info += "Enabled";
    #else
    device_info += "Disabled";
    #endif
    device_info += "\r\n";
    
    return device_info;
};

void CLI_service::Status(){
    cli->Print("Status\r\n");
}

void CLI_service::Bootloader(){
    reset_usb_boot(0, 0);
}

void CLI_service::Restart(){
    watchdog_enable(1, 1);  // Enable watchdog with loop time 1ms
    while (1);              // Loop until watchdog triggers reset
}

void CLI_service::Thread_statistics() {
    // Buffer to hold the runtime stats
    char runTimeStats[1024];

    // Generate the runtime stats
    vTaskGetRunTimeStats(runTimeStats);

    // Print the runtime stats
    cli->Print(runTimeStats);
}



void CLI_service::Bind(const std::string &command, std::function<void()> function, const std::string help_message){
    this->cli->Bind(command, function, help_message);
    this->registered_commands.push_back(command);
}
void CLI_service::Bind(
    const std::string &command, 
    std::function<void(std::vector<std::string>)> function, 
    const std::string help_message,
    const std::string arguments
){
    this->cli->Bind(command, function, help_message + dye::light_black("\r\n\tUsage: " + arguments));
    this->registered_commands.push_back(command+"*");
}
void CLI_service::Print(const std::string &message){
    this->cli->Print(message);
}
void CLI_service::Print_ln(const std::string &message){
    this->cli->Print(message + "\r\n");
}

void CLI_service::Print_error(const std::string &message){
    this->cli->Print(dye::red("error: " + message) + "\r\n");
}
void CLI_service::Print_notice(const std::string &message){
    if(this->cli->Interactive()){
        this->cli->Print(dye::light_black(message) + "\r\n");
    }
}

bool CLI_service::Check_argument_count(const std::vector<std::string>& args, size_t minimum_arguments, size_t maximum_arguments){
    if(args.size() < minimum_arguments){
        this->Print_error(emio::format("not enough arguments, minimum is {}",minimum_arguments));
        return false;
    }
    if(args.size() > maximum_arguments && maximum_arguments != SIZE_MAX){
        this->Print_error(emio::format("too many arguments, maximum is {}",maximum_arguments));
        return false;
    }
    
    return true;
}

bool CLI_service::Parse_argument(const std::string& arg, float& parsed_value){
    auto [ptr, ec] = std::from_chars(
        arg.data(),
        arg.data() + arg.size(),
        parsed_value
    );
    
    // tests also if the string was parsed till the end.
    if (ec != std::errc() || ptr != arg.data() + arg.size()) {
        this->Print_error("invalid argument (expected a float)");
        return false;
    }
    
    return true;
}

bool CLI_service::Parse_argument(const std::string& arg, int& parsed_value){
    auto [ptr, ec] = std::from_chars(
        arg.data(),
        arg.data() + arg.size(),
        parsed_value
    );
    
    // tests also if the string was parsed till the end.
    if (ec != std::errc() || ptr != arg.data() + arg.size()) {
        this->Print_error("invalid argument (expected an integer)");
        return false;
    }
    
    return true;
}

bool CLI_service::Parse_argument(const std::string& arg, unsigned int& parsed_value){
    auto [ptr, ec] = std::from_chars(
        arg.data(),
        arg.data() + arg.size(),
        parsed_value
    );
    
    // tests also if the string was parsed till the end.
    if (ec != std::errc() || ptr != arg.data() + arg.size()) {
        this->Print_error("invalid argument (expected an integer)");
        return false;
    }
    
    return true;
}


bool CLI_service::Parse_argument(const std::string& arg, bool& parsed_value){
    if(arg == "true" || arg == "1" || arg == "y" || arg == "Y"){
        parsed_value = true;
        return true;
    }
    
    if(arg == "false" || arg == "0" || arg == "n" || arg == "N"){
        parsed_value = false;
        return true;
    }
    
    this->Print_error("invalid argument (expected a true/false)");
    return false;
}
