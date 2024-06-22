#include "cli.hpp"

CLI_service::CLI_service():cli(new CLI(0, 256, 32,"\033[94m>\033[0m ")){

    auto status = [this]()->void {
          Status();
    };

    auto device_info_print = [this]()->void {
          cli->Print(Device_info());
    };

    cli->Bind("status", status,"None");
    cli->Bind("device_info", device_info_print, "Constains version, build timestamp, git commit hash, etc.");
    cli->Bind("bootloader", [this]()->void { Bootloader(); }, "Reboots MCU into bootloader mode for fw update");
    cli->Bind("restart", [this]()->void { Restart(); }, "Restart MCU using watchdog");
    cli->Bind("thread_statistics", [this]()->void { Thread_statistics(); }, "Print statistics of FreeRTOS threads");

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

std::string CLI_service::Device_info(){
    char unique_id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];  // Each byte will be 2 hex digits, plus null terminator
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
        sprintf(&unique_id[i*2], "%02x", id.id[i]);
    }

    std::string device_info = "";
    device_info += emio::format("Device name: {}\r\n", DEVICE_NAME);
    device_info += emio::format("Unique ID: {}\r\n", unique_id);
    device_info += emio::format("Vendor: {}\r\n", VENDOR_NAME);
    device_info += emio::format("Build timestamp: {}\r\n", __TIMESTAMP__);
    device_info += emio::format("Firmware version: {}\r\n", FW_VERSION);
    device_info += emio::format("Git commit hash: {}\r\n", GIT_COMMIT_HASH);
    device_info += emio::format("Compiler: {} {}.{}.{}\r\n", COMPILER_NAME, __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
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
