/**
 * @file logger.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 01.06.2024
 */

#pragma once

#include <string>
#include <functional>

#include "tusb.h"
#include "emio/emio.hpp"

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <hardware/dma.h>

/**
 *  @brief  Basic logger which is used for printing messages to UART and USB
 *          Logger has only once static instance and is accessible from anywhere
 *          Messages can be colorized, but at default are not
 *          Message of logger is prefixed with timestamp with ms precision
 */
class Logger{
private:
    /**
     * @brief USb interface id which is used for printing to USB
     */
    inline static std::optional<uint8_t> usb_interface_id = {};

    /**
     * @brief UART instance which is used for printing to UART
     */
    inline static uart_inst_t * uart_instance = nullptr;

    inline static int dma_channel = 1;

    inline static std::string buffer = "";

public:
    /**
     * @brief Construct a new Logger object
     */
    Logger() = default;

    /**
     * @brief   Initialize uart peripheral for logging
     *
     * @param uart_instance UART instance which will be used for logging
     * @param tx_gpio       GPIO pin which will be used for TX
     * @param rx_gpio       GPIO pin which will be used for RX
     */
    static void Init_UART(uart_inst_t * uart_instance, uint tx_gpio, uint rx_gpio, uint baudrate = 115200);

    /**
     * @brief   Initialize USB peripheral for logging
     *
     * @param usb_interface_id  USB interface id which will be used for logging
     */
    static void Init_USB(uint usb_interface_id);

    /**
     * @brief Print message to UART and USB with timestamp
     *
     * @param message   Message which will be printed
     */
    static void Print(std::string message);

    /**
     * @brief Print message to UART and USB with color and timestamp
     *
     * @param message   Message which will be printed
     * @param colorizer Function which will colorize message
     */
    static void Print(std::string message, std::function<std::string(const std::string&)> colorizer);

    /**
     * @brief   Print message into UART and USB without any formatting or timestamp
     *
     * @param message   Message which will be printed
     */
    static void Print_raw(std::string message);

private:
    /**
     * @brief Get current timestamp with ms precision
     *
     * @return std::string  Timestamp in format [ssss.mmmm]
     */
    static std::string Timestamp();

    /**
     * @brief Print message to USB
     *
     * @param message   Message which will be printed
     */
    static void Print_to_USB(std::string &message);

    /**
     * @brief Print message to UART
     *
     * @param message   Message which will be printed
     */
    static void Print_to_UART(const std::string &message);
};
