/**
 * @file info.hpp
 * @author David Radek (david-radek(at)seznam(dot)cz)
 * @version 0.12
 * @date 19.06.2026
 * @brief this file contains strings that will be visible in the build binaries.
 *        These strings are to be used as information about the binary (metadata)
 */

#pragma once

#include "config.hpp"

#define INFO_STRINGS_STR(s)  #s
#define INFO_STRINGS_XSTR(s) INFO_STRINGS_STR(s)

// optimization needs to be turned off in order for the strings to remain in the resulting binary
#pragma GCC push_options
#pragma GCC optimize ("O0")

inline const char* __attribute__((used,retain)) info_strings[] = {
    "___INFO___ | version     | \""
        INFO_STRINGS_XSTR(FW_VERSION_MAJOR) "."
        INFO_STRINGS_XSTR(FW_VERSION_MINOR) "."
        INFO_STRINGS_XSTR(FW_VERSION_PATCH) "\"",
    #ifdef CONFIG_CONTROL_MODULE
    "___INFO___ | module type | 0x05",
    #elifdef CONFIG_SENSOR_MODULE
    "___INFO___ | module type | 0x06",
    #elifdef CONFIG_PUMP_MODULE
    "___INFO___ | module type | 0x07",
    #else
    "___INFO___ | module type | 0x00",
    #endif
    "___INFO___ | module name | " INFO_STRINGS_XSTR(CONFIG_MODULE_TYPE),
    "___INFO___ | git commit  | " INFO_STRINGS_XSTR(FW_GIT_COMMIT_HASH_STR),
    "___INFO___ | git branch  | " INFO_STRINGS_XSTR(FW_GIT_BRANCH),
    "___INFO___ | git dirty   | " INFO_STRINGS_XSTR(FW_GIT_DIRTY),
    "___INFO___ | build time  | " INFO_STRINGS_XSTR(__TIMESTAMP__),
    "___INFO___ | compiler    | " INFO_STRINGS_XSTR(FW_COMPILER_NAME),

    #if defined(CONFIG_LOGGER_UART) || defined(CONFIG_LOGGER_USB)
        #ifdef CONFIG_LOGGER_UART
        "___INFO___ | logger UART | true",
        #else
        "___INFO___ | logger UART | false",
        #endif
        #ifdef CONFIG_LOGGER_USB
        "___INFO___ | logger USB  | true",
        #else
        "___INFO___ | logger USB  | false",
        #endif
    "___INFO___ | logger lvl  | " INFO_STRINGS_XSTR(CONFIG_LOGGER_LEVEL),
    #else
    "___INFO___ | logger      | false",
    #endif

    #ifdef CONFIG_WATCHDOG
    "___INFO___ | watchdog    | true",
    #else
    "___INFO___ | watchdog    | false",
    #endif
    "___INFO___ | config file | " INFO_STRINGS_XSTR(FW_KCONFIG_FILE),
};

#pragma GCC pop_options
