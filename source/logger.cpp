#include "logger.hpp"

void Logger::Init_UART(){
    uart_init(uart0, 115200); // Initialize UART with a baud rate of 115200

    // Set the UART pins
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
}

void Logger::Print(std::string message){
    std::string text = Timestamp() + message + "\r\n";
    Print_to_USB(text);
    Print_to_UART(text);
}

void Logger::Print(std::string message, std::function<std::string(const std::string&)> colorizer){
    message = colorizer(message);
    Print(message);
}

void Logger::Print_to_USB(std::string &message){
    if (tud_cdc_n_connected(usb_interface_id)) {
        tud_cdc_n_write(usb_interface_id, message.c_str(), message.length());
        tud_cdc_n_write_flush(usb_interface_id);
    }
}

void Logger::Print_to_UART(const std::string &message){
    uart_puts(uart0, message.c_str());
}

std::string Logger::Timestamp(){
    return emio::format("[{:09.3f}] ", time_us_64()/1000000.0f);
}
