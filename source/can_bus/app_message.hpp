/**
 * @file app_message.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 14.08.2024
 */

#pragma once

#include "can_message.hpp"

#include "codes/codes.hpp"

/**
 * @brief  Subset of CAN message, which is used for application communication
 *         Uses only extended can frame format, id holds information about:
 *             Target/source module, instance enumeration of module and type of message
 *         Extended id is composed of:
 *             1 bit  - Emergency flag
 *             12 bit - Message type
 *             4 bit  - Reserved for future use
 *             8 bit  - Module ID
 *             4 bit  - Instance enumeration
 */
class Application_message : public CAN::Message {
    /**
     * @brief Header of message which is stored is frame ID
     *          Contains information about target/source module, instance and message type
     */
    struct __attribute__((packed)) __attribute__((__may_alias__)) Header{
        Codes::Instance     Instance_enumeration : 4 = Codes::Instance::Undefined;
        Codes::Module       Module_type : 8 = Codes::Module::Undefined;
        uint8_t             Reserved : 4 = 0x0;
        Codes::Message_type Message_type : 12 = Codes::Message_type::Undefined;
        bool                Emergency_flag : 1 = false;
    };

    /**
     * @brief   Construct a new Application_message object, subset of CAN message with extended frame format without data
     *          Protected, to force use of variant which read data of Base module and cannot be used modified
     *
     * @param module_type           Typeof module, which should receive the message or which is sending it
     * @param instance_enumeration  Instance enumeration of module, which should receive the message or which is sending it
     * @param message_type          Type of message, which is sent, determine purpose and handling of message by corresponding module
     */
    Application_message(Codes::Module module_type, Codes::Instance instance_enumeration, Codes::Message_type message_type);

    /**
     * @brief   Construct a new Application_message object, subset of CAN message with extended frame format with data
     *          Protected, to force use of variant which read data of Base module and cannot be used modified
     *
     * @param module_type           Typeof module, which should receive the message or which is sending it
     * @param instance_enumeration  Instance enumeration of module, which should receive the message or which is sending it
     * @param message_type          Type of message, which is sent, determine purpose and handling of message by corresponding module
     * @param data                  Data which are used to initialize data section of message, up to 8 bytes
     */
    Application_message(Codes::Module module_type, Codes::Instance instance_enumeration, Codes::Message_type message_type, etl::vector<uint8_t, 8> data);

public:
    /**
     * @brief   Construct a new Application_message object, subset of CAN message with extended frame format without data
     *          Public, allows creation of message with only message type, other parameters are obtained from Base_module class
     *
     * @param message_type  Type of message, which is sent, determine purpose and handling of message by corresponding module
     */
    explicit Application_message(Codes::Message_type message_type);

    /**
     * @brief   Construct a new Application_message object, subset of CAN message with extended frame format with data
     *          Public, allows creation of message with only message type and data, other parameters are obtained from Base_module class
     *
     * @param message_type  Type of message, which is sent, determine purpose and handling of message by corresponding module
     * @param data          Data which are used to initialize data section of message, up to 8 bytes
     */
    Application_message(Codes::Message_type message_type, etl::vector<uint8_t, 8> data);

    /**
     * @brief   Construct a new Application_message object, subset of CAN message with extended frame format with data based on existing can message
     *          TODO: Should be checked if existing message is valid application message (is extended frame for example)
     *
     * @param message   Existing CAN message, which is used to create Application message
     */
    Application_message(CAN::Message message);

    /**
     * @brief   Determines type of module, which should receive the message or which is sending it based on corresponding part of message ID
     *
     * @return  Codes::Module   Type of module, which should receive the message or which is sending it
     */
    Codes::Module Module_type() const;

    /**
     * @brief   Determines instance enumeration of module, which should receive the message or which is sending it based on corresponding part of message ID
     *
     * @return  Codes::Instance Instance enumeration of module, which should receive the message or which is sending it
     */
    Codes::Instance Instance_enumeration() const;

    /**
     * @brief   Determines type of message, which is sent, determine purpose and handling of message by corresponding module based on corresponding part of message ID
     *
     * @return  Codes::Message_type   Type of message, which is sent, determine purpose and handling of message by corresponding module
     */
    Codes::Message_type Message_type() const;
};
