/**
 * @file logger.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 01.06.2024
 */

#pragma once

#include <string>
#include <functional>
#include <map>

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
    /**
     * @brief   Severity level of messages
     */
    enum class Level {
        Trace,
        Debug,
        Notice,
        Warning,
        Error,
        Critical
    };

    /**
     * @brief   Output color mode of logger
     */
    enum class Color_mode {
        None,       // No colors
        Prefix,     // Only level prefix is colored
        Timestamp,  // Without prefix with colorized timestamp
        Text,       // Without prefix with message colored
        Full        // Entire message is colored
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

    /**
     * @brief   DMA channel which is used for printing to UART
     */
    inline static int dma_channel = 1;

    /**
     * @brief   Buffer for storing messages before printing via DMA
     */
    inline static std::string buffer = "";

    /**
     * @brief   Current log level, messages with lower level than this will not be printed
     */
    inline static Level current_log_level = Level::Trace;

    /**
     * @brief   Current color mode of logger
     */
    inline static Color_mode color_mode = Color_mode::Full;

    /**
     * @brief   Color code reset for terminal
     */
    inline const static std::string color_reset = "\033[0m";

    /**
     * @brief   Map of colors for each level
     */
    const static inline std::map<Level, std::string> level_colors = {
        {Level::Trace,    "\033[37m"},
        {Level::Debug,    "\033[34m"},
        {Level::Notice,   "\033[32m"},
        {Level::Warning,  "\033[33m"},
        {Level::Error,    "\033[31m"},
        {Level::Critical, "\033[35m"}
    };

    /**
     * @brief   Map of prefixes for each level
     */
    const static inline std::map<Level, std::string> level_prefixes = {
        {Level::Trace,    "TRC "},
        {Level::Debug,    "DBG "},
        {Level::Notice,   "NOT "},
        {Level::Warning,  "WAR "},
        {Level::Error,    "ERR "},
        {Level::Critical, "CRT "}
    };

public:

    /**
     * @brief Construct a new Logger object with defined level
     *
     * @param level Messages with lower level than this will not be printed
     */
    explicit Logger(Level level = Level::Notice, Color_mode color_mode = Color_mode::None);

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
