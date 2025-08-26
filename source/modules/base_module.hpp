/**
 * @file base_module.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 14.08.2024
 */

#pragma once

#include "codes/codes.hpp"
#include "components/memory.hpp"
#include "components/enumerator.hpp"
#include "components/memory/AT24Cxxx.hpp"
#include "logger.hpp"
#include "components/common_core.hpp"
#include "threads/can_thread.hpp"
#include "threads/test_thread.hpp"
#include "threads/heartbeat_thread.hpp"
#include "config.hpp"
#include "hal/gpio/gpio.hpp"
#include "hal/adc/adc_channel.hpp"

namespace fra = cpp_freertos;

class Common_thread;

/**
 * @brief   Base interface class for all modules in device
 *          Contains parts shared by all modules like:
 *              - CAN bus manager thread responsible for handling of CAN Bus peripheral
 *              - Common thread which dispatches received messages to router
 *              - Common core which is component present in all modules and is responsible for processing of messages which must be supported by all modules
 *          Also contains information about module type and instance enumeration, this are accessible via
 *              static methods using somewhat hidden and dirty singleton pattern
 *          Every module class should be derived from this class
 */
class Base_module {
protected:
    /**
     * @brief Pointer to this class used for singleton pattern accessing of module type and instance enumeration
     */
    static inline Base_module * instance;

    Enumerator * const enumerator;

    /**
     * @brief Type of module, initialized by derived class calling constructor of this class with its type
     */
    const Codes::Module module_type = Codes::Module::Any;

    /**
     * @brief Instance enumeration of module, initialized by derived class calling constructor of this class with its type
     */
    Codes::Instance instance_enumeration = Codes::Instance::Undefined;

    /**
     * @brief  Main I2C bus of system connected to sensors and detectors
     */
    I2C_bus * const i2c;

    /**
     * @brief  Pointer to EEPROM storage which is used for storing persistent data (calibration, etc)
     */
    EEPROM_storage * const memory;

    /**
     * @brief   Mutex for ADC access, this is used to prevent multiple threads accessing ADC at the same time
     *              Prevents measurement distortion due to different settings used on each channel
     */
    fra::MutexStandard * const adc_mutex;

    /**
     * @brief  Pointer to CAN bus manager thread which is responsible for handling of CAN Bus peripheral
     */
    CAN_thread * const can_thread;

    /**
     * @brief  Pointer to common thread which dispatches received messages to router
     *         This thread also perform tasks resulting from this messages, message router will not spawn new thread for task processing
     */
    Common_thread * const common_thread;

    /**
     * @brief  Pointer to common core which is component present in all modules and is responsible for processing of messages which must be supported by all modules
     */
    Common_core * const common_core;

    /**
     * @brief   Pointer to heartbeat thread which is responsible for blinking of green LED
     */
    Heartbeat_thread * const heartbeat_thread;

    /**
     * @brief  Pointer to yellow LED GPIO pin, this LED is optional
     */
    const std::optional<GPIO * const> yellow_led = {};

    /**
     * @brief   ADC channel reading version dividers voltage of module
     */
    ADC_channel * version_voltage_channel = nullptr;

protected:
    /**
     * @brief Construct a new Base_module object, must be called from constructor of derived class
     *
     * @param module_type   Type of module which is derived from this class
     * @param instance_type Instance enumeration of module which is derived from this class
     * @param green_led_pin GPIO pin of green LED
     */
    Base_module(Codes::Module module_type, Enumerator * const enumerator, uint green_led_pin, uint i2c_sda, uint i2c_scl);

    /**
     * @brief Construct a new Base_module object, must be called from constructor of derived class
     *
     * @param module_type   Type of module which is derived from this class
     * @param instance_type Instance enumeration of module which is derived from this class
     * @param green_led_pin  GPIO pin of green LED
     * @param yellow_led_pin GPIO pin of yellow LED
     */
    Base_module(Codes::Module module_type, Enumerator * const enumerator, uint green_led_pin, uint i2c_sda, uint i2c_scl, uint yellow_led_pin);

private:
    /**
     * @brief Construct a new Base_module object, called internally from public constructors
     *
     * @param module_type   Type of module which is derived from this class
     * @param instance_type Instance enumeration of module which is derived from this class
     * @param green_led_pin GPIO pin number of green LED passed to heartbeat thread
     * @param yellow_led    Optional pointer to GPIO object representing yellow LED
     */
    Base_module(Codes::Module module_type, Enumerator * const enumerator, uint green_led_pin, uint i2c_sda, uint i2c_scl, std::optional<GPIO * const> yellow_led);

public:
    /**
     * @brief Method implemented by derived class, which should setup all module specific components and functionality
     */
    virtual void Setup_components() = 0;

    /**
     * @brief Determine module type of this module
     */
    static Codes::Module Module_type();

    /**
     * @brief Determine instance enumeration of this module
     */
    static Codes::Instance Instance_enumeration();

    /**
     * @brief Wrapper function to send message to CAN bus via can_thread
     *
     * @param message  Message to be sent
     * @return uint    Number of messages in queue, if 0 then was send immediately
     */
    static uint Send_CAN_message(App_messages::Base_message &message);

    /**
     * @brief Wrapper function to send message to CAN bus via can_thread
     *
     * @param message  Message to be sent
     * @return uint    Number of messages in queue, if 0 then was send immediately
     */
    static uint Send_CAN_message(CAN::Message const &message);

    /**
     * @brief   Retrieves current temperature of board
     *          Implemented by every board module
     *
     * @return float    Temperature of board in Celsius
     */
    virtual std::optional<float> Board_temperature() = 0;

    /**
     * @brief   Get voltage on version pin of module, this is used to determine hardware version of module
     *
     * @return  std::optional<float>    Voltage on version divider resistor which is mapped to hw version
     */
    virtual std::optional<float> Version_voltage() const ;

    /**
     * @brief Get pointer to this class instance using "singleton" pattern
     */
    static Base_module * Instance();

};
