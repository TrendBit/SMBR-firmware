#include "can_bus.hpp"
#include "can2040.h"
#include "can_bus/can_message.hpp"
#include <algorithm>

CAN::Bus::Bus(unsigned int gpio_rx, unsigned int gpio_tx, unsigned int bitrate, uint pio_num) :
    pio_number(pio_num),
    sys_clock(125000000){
    uint32_t sys_clock = clock_get_hz(clk_sys);

    can2040_setup(&handler, pio_number);

    instances[pio_number] = this;

    can2040_callback_config(&handler, &Bus::Callback_handler);

    Enable_IRQ<5>();
    can2040_start(&handler, sys_clock, bitrate, gpio_rx, gpio_tx);
}

void CAN::Bus::Callback( uint32_t notify, struct can2040_msg *msg){
    IRQ_type type;
    if(not (msg->id & CAN2040_ID_EFF)){
        return;
    }
    if (notify == CAN2040_NOTIFY_RX) {
        type     = IRQ_type::RX;
        received_messages.emplace(Message(msg));
    } else if (notify == CAN2040_NOTIFY_TX) {
        type     = IRQ_type::TX;
    } else {
        type     = IRQ_type::Error;
    }
    Emit(type);
}

void CAN::Bus::Callback_handler(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg){
    if (instances.find(cd->pio_num) != instances.end()) {
        instances[cd->pio_num]->Callback(notify, msg);
    }
}

void CAN::Bus::Handle_PIO_IRQ(){
    can2040_pio_irq_handler(&handler);
}

bool CAN::Bus::Transmit(struct can2040_msg *msg){
    return !can2040_transmit(&handler, msg);
}

bool CAN::Bus::Transmit(Message const &message){
    can2040_msg msg = message.to_msg();
    return !can2040_transmit(&handler, &msg);
}

bool CAN::Bus::Transmit_available(){
    return can2040_check_transmit(&handler);
}

uint8_t CAN::Bus::Received_queue_size(){
        return received_messages.size();
    }

std::optional<CAN::Message> CAN::Bus::Receive() {
    if(!received_messages.empty()){
        std::optional<CAN::Message> message(received_messages.front());
        received_messages.pop();
        return message;
    } else {
        return {};
    }
}
