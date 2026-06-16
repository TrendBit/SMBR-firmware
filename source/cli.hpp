/**
 * @file bbx_cli.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 22.11.2023
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

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
    
    std::vector<std::string> registered_commands;
    
    void Print_command_list();

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
    
    /**
     * @brief   Bind function to CLI command
     *
     * @param command   Command string
     * @param function  Function to be called when command is received
     * @param help      Help string
     */
    void Bind(const std::string &command, std::function<void()> function, const std::string help_message);

    /**
     * @brief   Bind function to CLI command
     *
     * @param command   Command string
     * @param function  Function to be called when command is received, 
     *                  this variant receives arguments from CLI
     * @param help      Help string
     * @param arguments Usage string explaining what are the arguments of this command.
     *                  Use simple text for mandatory arguments, wrap optional in [argument]?
     *                  Commonly typed arguments should have their type as argument(type)
     */
    void Bind(
        const std::string &command, 
        std::function<void(std::vector<std::string>)> function, 
        const std::string help_message,
        const std::string arguments
    );
    
    /**
     * @brief  Print message to CLI, prints data event when interactive mode is disabled
     *
     * @param message  Message to be printed
     */
    void Print(const std::string &message);
    
    /**
     * @brief  Print message to CLI, prints data event when interactive mode is disabled. 
     *         Automatically adds newline.
     *
     * @param message  Message to be printed
     */
    void Print_ln(const std::string &message);
    
    /**
     * @brief  Prints an error message to CLI. Automatically adds newline.
     *
     * @param message  Message to be printed
     */
    void Print_error(const std::string &message);
    
    /**
     * @brief  Prints a notice message to CLI that should be used to
     *         signalize a successfull command to the user.
     *         Does not appear in non-interactive mode.
     *         Automatically adds newline.
     *
     * @param message  Message to be printed
     */
    void Print_notice(const std::string &message);
    
    /**
     * @brief Checks if the given args vector contains the right amount of arguments.
     *        If not, it prints out an error message to the cli.
     * 
     * @param args                The commands arguments
     * @param minimum_arguments   How many arguments should the command have at minimum
     * @param minimum_arguments   How many arguments should the command have at maximum (optional)
     * 
     * @return true     The argument count is OK
     * @return false    The argument count is not correct and an error has been printed
     */
    bool Check_argument_count(
        const std::vector<std::string>& args, 
        size_t minimum_arguments, 
        size_t maximum_arguments = SIZE_MAX
    );
    
    /**
     * @brief Parse the given argument as a given type. Outputs an error to the cli if there is something wrong.
     * 
     * @param arg           The argument that should be parsed
     * @param parsed_value  An output of the method, containing the parsed argument
     * 
     * @return true     Parsing of the argument was successfull
     * @return false    Parsing of the argument was not successfull, and an error has been printed
     * 
     * @note This funtion was not inlined and templated because it uses large header files 
     *       that would not be used anywhere else. The given types should be enough for most uses.
     */
    bool Parse_argument(const std::string& arg, float& parsed_value);
    
    /**
    * @brief Parse the given argument as a given type. Outputs an error to the cli if there is something wrong.
    * 
    * @param arg           The argument that should be parsed
    * @param parsed_value  An output of the method, containing the parsed argument
     * 
     * @return true     Parsing of the argument was successfull
     * @return false    Parsing of the argument was not successfull, and an error has been printed
     * 
     * @note This funtion was not inlined and templated because it uses large header files 
     *       that would not be used anywhere else. The given types should be enough for most uses.
     */
    bool Parse_argument(const std::string& arg, int& parsed_value);
    
    /**
    * @brief Parse the given argument as a given type. Outputs an error to the cli if there is something wrong.
    * 
    * @param arg           The argument that should be parsed
    * @param parsed_value  An output of the method, containing the parsed argument
     * 
     * @return true     Parsing of the argument was successfull
     * @return false    Parsing of the argument was not successfull, and an error has been printed
     * 
     * @note This funtion was not inlined and templated because it uses large header files 
     *       that would not be used anywhere else. The given types should be enough for most uses.
     */
    bool Parse_argument(const std::string& arg, unsigned int& parsed_value);
    
    /**
    * @brief Parse the given argument as a given type. Outputs an error to the cli if there is something wrong.
    * 
    * @param arg           The argument that should be parsed
    * @param parsed_value  An output of the method, containing the parsed argument
     * 
     * @return true     Parsing of the argument was successfull
     * @return false    Parsing of the argument was not successfull, and an error has been printed
     * 
     * @note This funtion was not inlined and templated because it uses large header files 
     *       that would not be used anywhere else. The given types should be enough for most uses.
     */
    bool Parse_argument(const std::string& arg, bool& parsed_value);
};