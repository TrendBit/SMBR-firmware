/**
 * @file base_module.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 14.08.2024
 */

#pragma once

#include "codes/codes.hpp"
#include "logger.hpp"
#include "threads/can_thread.hpp"
#include "components/common_core.hpp"

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

    /**
     * @brief Type of module, initialized by derived class calling constructor of this class with its type
     */
    const Codes::Module module_type = Codes::Module::Any;

    /**
     * @brief Instance enumeration of module, initialized by derived class calling constructor of this class with its type
     */
    Codes::Instance instance_enumeration = Codes::Instance::Undefined;

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

public:
    /**
     * @brief Construct a new Base_module object, must be called from constructor of derived class
     *
     * @param module_type   Type of module which is derived from this class
     * @param instance_type Instance enumeration of module which is derived from this class
     */
    Base_module(Codes::Module module_type, Codes::Instance instance_type);

    /**
     * @brief Construct a new Base_module object, must be called from constructor of derived class
     *
     * @param module_type   Type of module which is derived from this class
     * @param instance_type Instance enumeration of module which is derived from this class
     */
    Base_module(Codes::Module module_type, Codes::Instance instance_type, uint green_led_pin);

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

protected:
    /**
     * @brief Get pointer to this class instance using "singleton" pattern
     */
    static Base_module * Instance();

};
