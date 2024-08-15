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
    static uint8_t const usb_interface_id = 1;
public:
    /**
     * @brief Construct a new Logger object
     */
    Logger() = default;

    /**
     * @brief Initialize uart peripheral for logging
     */
    static void Init_UART();

    /**
     * @brief Print message to UART and USB
     *
     * @param message   Message which will be printed
     */
    static void Print(std::string message);

    /**
     * @brief Print message to UART and USB with color
     *
     * @param message   Message which will be printed
     * @param colorizer Function which will colorize message
     */
    static void Print(std::string message, std::function<std::string(const std::string&)> colorizer);

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
