#include "can_message.hpp"
#include <algorithm>

CAN::Message::Message(uint32_t id, uint8_t length, bool extended, bool remote_request)
    :
    id(id),
    extended(extended),
    remote_request(remote_request),
    data(etl::vector<uint8_t, 8>(length, 0)){ }

CAN::Message::Message(uint32_t id, etl::vector<uint8_t, 8> data, bool extended, bool remote_request)
    :
    id(id),
    extended(extended),
    remote_request(remote_request),
    data(data){ }

CAN::Message::Message(can2040_msg *msg)
    : id(msg->id & 0x1fffffff),
    extended(msg->id & CAN2040_ID_EFF),
    remote_request(msg->id & CAN2040_ID_RTR),
    data(etl::vector<uint8_t, 8>(msg->dlc, 0)){
    std::copy(msg->data, msg->data + msg->dlc, data.begin());
}

uint32_t CAN::Message::ID() const {
    return id;
}

bool CAN::Message::Extended() const {
    return extended;
}

bool CAN::Message::Remote() const {
    return remote_request;
}

can2040_msg CAN::Message::to_msg() const {
    struct can2040_msg can_message = { };
    can_message.id = id;

    if (extended) {
        can_message.id |= CAN2040_ID_EFF;
    }

    if (remote_request) {
        can_message.id |= CAN2040_ID_RTR;
    }

    can_message.dlc = data.size();
    std::copy(data.begin(), data.end(), can_message.data);

    return can_message;
}
