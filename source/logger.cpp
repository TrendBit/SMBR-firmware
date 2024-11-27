#include "logger.hpp"

void Logger::Init_UART(uart_inst_t * uart_instance, uint tx_gpio, uint rx_gpio, uint baudrate)
{
    Logger::uart_instance = uart_instance;
    uart_init(uart_instance, baudrate);

    // Set the UART pins
    gpio_set_function(tx_gpio, GPIO_FUNC_UART);
    gpio_set_function(rx_gpio, GPIO_FUNC_UART);
}

void Logger::Init_USB(uint usb_interface_id)
{
    Logger::usb_interface_id = usb_interface_id;
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

void Logger::Print_raw(std::string message){
    Print_to_USB(message);
    Print_to_UART(message);
}

void Logger::Print_to_USB(std::string &message){
    if (usb_interface_id.has_value()) {
        if (tud_cdc_n_connected(usb_interface_id.value())) {
            tud_cdc_n_write(usb_interface_id.value(), message.c_str(), message.length());
            tud_cdc_n_write_flush(usb_interface_id.value());
        }
    }
}

void Logger::Print_to_UART(const std::string &message){
    if (uart_instance) {
        uart_puts(uart_instance, message.c_str());
        uart_tx_wait_blocking(uart_instance);
    }
}

std::string Logger::Timestamp(){
    return emio::format("[{:09.3f}] ", time_us_64()/1000000.0f);
}
