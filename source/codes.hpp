/**
 * @file codes.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 02.06.2024
 */

#pragma once

namespace Codes {
/**
     * @brief 12-bit identifier of device, together with 4 bit device enumeration creates, Unique ID of device
     */
enum class Device : uint16_t {
    Undefined = 0x000,
    TestBed   = 0x001,
};

enum class Component {
    Undefined,
    Core,
    CAN_serial,
};

/**
     * @brief   11-bit identifier from CAN message header (ID)
     */
enum class Command : uint16_t {
    Undefined                = 0x000,
    Serial_probe             = 0x321,
    Serial_ID_respond        = 0x323,
    Serial_port_confirmation = 0x322,
    Serial_port_0_RX         = 0x180,
    Serial_port_0_TX         = 0x181,
    Discover_probe           = 0x420,
    Discover_respond         = 0x422,
};

}
