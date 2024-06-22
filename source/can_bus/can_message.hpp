/**
 * @file can_message.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 01.06.2024
 */

#pragma once

extern "C" {
    #include "can2040.h"
}

#include "etl/vector.h"

namespace CAN {
class Message {
private:

    uint32_t id;

    bool extended = false;

    bool remote_request = false;

public:
    etl::vector<uint8_t, 8> data;

public:
    explicit Message(uint32_t id, uint8_t length, bool extended = false, bool remote_request = false);

    explicit Message(uint32_t id, etl::vector<uint8_t, 8> data, bool extended = false, bool remote_request = false);

    explicit Message(can2040_msg *msg);

    uint32_t ID() const;

    bool Extended() const;

    bool Remote() const;

    can2040_msg to_msg() const;
};
}
