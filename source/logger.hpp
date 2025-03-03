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
public:
    enum class Level {
        Trace,
        Debug,
        Notice,
        Warning,
        Error,
        Critical
    };


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

    inline static Level current_log_level = Level::Trace;

public:
    /**
     * @brief Construct a new Logger object with default Notice level
     */
    Logger(){ Logger(Level::Notice); };

    /**
     * @brief Construct a new Logger object with defined level
     *
     * @param level Messages with lower level than this will not be printed
     */
    explicit Logger(Level level);

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
     * @param level     Level of message
     */
    static void Print(std::string message, Level level = Level::Notice);

    /**
     * @brief Print message to UART and USB with color and timestamp
     *
     * @param message   Message which will be printed
     * @param colorizer Function which will colorize message
     * @param level     Level of message
     */
    static void Print(std::string message, std::function<std::string(const std::string&)> colorizer, Level level = Level::Notice);

    /**
     * @brief Print trace level message
     * @param message Message to print
     */
    static void Trace(std::string message) { Print(message, Level::Trace); }

    /**
     * @brief Print debug level message
     * @param message Message to print
     */
    static void Debug(std::string message) { Print(message, Level::Debug); }

    /**
     * @brief Print notice level message
     * @param message Message to print
     */
    static void Notice(std::string message) { Print(message, Level::Notice); }

    /**
     * @brief Print warning level message
     * @param message Message to print
     */
    static void Warning(std::string message) { Print(message, Level::Warning); }

    /**
     * @brief Print error level message
     * @param message Message to print
     */
    static void Error(std::string message) { Print(message, Level::Error); }

    /**
     * @brief Print critical level message
     * @param message Message to print
     */
    static void Critical(std::string message) { Print(message, Level::Critical); }

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
