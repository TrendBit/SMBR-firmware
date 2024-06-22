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

class Logger{
private:
    static uint8_t const usb_interface_id = 1;
public:
    Logger() = default;

    static void Init_UART();

    static void Print(std::string message);

    static void Print(std::string message, std::function<std::string(const std::string&)> colorizer);

private:

    static std::string Timestamp();

    static void Print_to_USB(std::string &message);

    static void Print_to_UART(const std::string &message);
};
