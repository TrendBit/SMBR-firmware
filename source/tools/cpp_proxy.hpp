#pragma once
#ifdef __cplusplus

#include <string>
    /**
    * @brief Returns a string containing the module type with prefix "SMPBR - "
    * @return string
    */
    const std::string Generate_product_name_cpp();
    
    /**
    * @brief Returns a string containing the module UID formated to fit 12 characters
    * @return string
    */
    const std::string Generate_device_serial_cpp();
    
    extern "C" {
#endif

/**
 * @brief Returns a string containing the module type with prefix "SMPBR - "
 * @return string of length max 31 characters + null character
 */
const char* Generate_product_name();

/**
 * @brief Returns a string containing the module UID formated to fit 12 characters
 * @return string of length max 31 characters + null character
 */
const char* Generate_device_serial();

#ifdef __cplusplus
    }
#endif
