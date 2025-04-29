#include "logger.hpp"

Logger::Logger(Level level, Color_mode color_mode){
    current_log_level = level;
    Logger::color_mode = color_mode;
}

void Logger::Init_UART(uart_inst_t * uart_instance, uint tx_gpio, uint rx_gpio, uint baudrate)
{
    Logger::uart_instance = uart_instance;
    uart_init(uart_instance, baudrate);

    // Set the UART pins
    gpio_set_function(tx_gpio, GPIO_FUNC_UART);
    gpio_set_function(rx_gpio, GPIO_FUNC_UART);

    static dma_channel_config dma_channel_config;
    uint instance_number = uart_get_index(uart_instance);
    uint dreq = instance_number ? DREQ_UART1_TX : DREQ_UART0_TX;
    uart_hw_t *uart_hw = instance_number ? uart1_hw : uart0_hw;

    dma_channel = dma_claim_unused_channel(true);

    dma_channel_config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&dma_channel_config, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_channel_config, true);
    channel_config_set_write_increment(&dma_channel_config, false);
    channel_config_set_dreq(&dma_channel_config, dreq);
    dma_channel_set_config(dma_channel, &dma_channel_config, false);
    dma_channel_set_write_addr(dma_channel, &uart_hw->dr, false);
    dma_channel_start(dma_channel);
}

void Logger::Init_USB(uint usb_interface_id)
{
    Logger::usb_interface_id = usb_interface_id;
}

void Logger::Print(std::string message, Level level){
    if (level < current_log_level) {
        return;
    }

    const std::string& color = level_colors.at(level);
    const std::string& prefix = level_prefixes.at(level);

    std::string text;
    switch (color_mode) {
        case Color_mode::None:
            text = prefix + Timestamp() + message;
            break;
        case Color_mode::Prefix:
            text = color + prefix + color_reset + Timestamp() + message;
            break;
        case Color_mode::Timestamp:
            text = color + Timestamp() + color_reset + message;
            break;
        case Color_mode::Text:
            text = Timestamp() + color + message + color_reset;
            break;
        case Color_mode::Full:
            text = color + prefix + Timestamp() + message + color_reset;
            break;
    }

    text += "\r\n";
    Print_to_USB(text);
    Print_to_UART(text);
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
        if(dma_channel_is_busy(dma_channel)){
            dma_channel_wait_for_finish_blocking(dma_channel);
        }
        buffer = message;
        dma_channel_transfer_from_buffer_now(dma_channel, buffer.data(), buffer.length());
    }
}

std::string Logger::Timestamp(){
    return emio::format("[{:09.3f}] ", time_us_64()/1000000.0f);
}
