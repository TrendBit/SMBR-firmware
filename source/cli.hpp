/**
 * @file bbx_cli.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 22.11.2023
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "tools/cli.hpp"
#include "tools/color.hpp"
#include "rtos/lamda_thread.hpp"
#include "rtos/wrappers.hpp"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/watchdog.h"

#define DEVICE_NAME "SMPBR - TestBed"
#define VENDOR_NAME "TrendBit s.r.o."

#ifndef FW_VERSION_MAJOR
    #define FW_VERSION_MAJOR 0
#endif

#ifndef FW_VERSION_MINOR
    #define FW_VERSION_MINOR 0
#endif

#ifndef FW_VERSION_PATCH
    #define FW_VERSION_PATCH 0
#endif

#ifndef FW_GIT_COMMIT_HASH_STR
    #define FW_GIT_COMMIT_HASH_STR "unknown"
#endif

#ifndef FW_COMPILER_NAME
    #define FW_COMPILER_NAME "unknown"
#endif

class CLI_service{
private:
    /**
     * @brief   CLI interface through which device could be controlled
     */
    CLI * const cli;

    /**
     * @brief   Thread which handle CLI service
     */
    fra::Thread * cli_service_thread;

public:
    CLI_service();

    /**
     * @brief  Return information about device
     *
     * @return std::string  Information about device
     */
    std::string Device_info();

    /**
     * @brief Change operational mode of status LED (red) via CLI based on first parametr of command
     *
     * @param state 1 to enable heartbeat LED (5Hz), 0 to disable LED blinking
     */
    void Heartbeat_CLI(std::vector<std::string> args);

    /**
     * @brief Print status of device into CLI
     */
    void Status();

    /**
     * @brief   Print statistics of FreeRTOS threads
     */
    void Thread_statistics();

    /**
     * @brief   Put MCU into bootloader mode in order to update firmware
     */
    void Bootloader();

    /**
     * @brief   Restart MCU, using watchdog
     */
    void Restart();

};
