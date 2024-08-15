#include "can_thread.hpp"
#include "can_bus/can_bus.hpp"
#include "logger.hpp"

CAN_thread::CAN_thread()
    : Thread("can_thread", 1000, 8),
    can_bus(CAN::Bus(5, 4, 100000, 0))
{
    Start();
};

void CAN_thread::Run(){
    while (true) {
        CAN::Bus::IRQ_type irq_type = can_bus.Wait_for_any<CAN::Bus::IRQ_type>();

        if(irq_type == CAN::Bus::IRQ_type::TX){ // Message was transmitted
            Logger::Print("CAN TX IRQ");
            if (not tx_queue.empty()) {
                Retransmit();
            }
        } else if(irq_type == CAN::Bus::IRQ_type::RX){  // Message was received
            Logger::Print("CAN RX IRQ");
            auto message = can_bus.Received();
            if (not message.has_value()) {
                Logger::Print("CAN message not found after RX IRQ");
            } else {
                Receive(message.value());
            }
        } else if (irq_type == CAN::Bus::IRQ_type::Error){  // Error occurred
            Logger::Print("CAN Error IRQ");
        } else {
            Logger::Print("CAN Unknown IRQ");   // Incorrect IRQ type evaluated
        }
    }
};

uint CAN_thread::Send(CAN::Message const &message){
    if(tx_queue.empty()){
        if(can_bus.Transmit_available()){
            Logger::Print("CAN available");
            if (can_bus.Transmit(message)){
                Logger::Print("CAN message sent");
            } else {
                Logger::Print("CAN message not sent");
            }
        } else {
            Logger::Print(emio::format("CAN not available, message queued, size: {}, available {}", tx_queue.size(), tx_queue.available()));
            tx_queue.push(message);
        }
    } else {
        Logger::Print(emio::format("CAN queue not empty, message queued, size: {}, available {}", tx_queue.size(), tx_queue.available()));
        if (tx_queue.full()) {
            Logger::Print("CAN TX queue full, message dropped");
            return 0;
        } else {
            tx_queue.push(message);
        }
    }
    return tx_queue.available();
};

void CAN_thread::Receive(CAN::Message const &message){
    if(rx_queue.full()){
        Logger::Print("CAN RX queue full, message dropped");
        return;
    }
    rx_queue.push(message);
    Logger::Print(emio::format("CAN message received, queue size: {}, available: {}", rx_queue.size(), rx_queue.available()));
};

uint8_t CAN_thread::Retransmit(){
    uint8_t retransmitted = 0;
    while((can_bus.Transmit_available()) and (not tx_queue.empty())){
        CAN::Message message = tx_queue.front();
        tx_queue.pop();
        can_bus.Transmit(message);
        retransmitted++;
    }
    Logger::Print(emio::format("CAN retransmitted: {}", retransmitted));
    return retransmitted;
};

uint32_t CAN_thread::Received_messages(){
    return rx_queue.size();
};

bool CAN_thread::Message_available() const{
    return not rx_queue.empty();
};

std::optional<CAN::Message> CAN_thread::Read_message(){
    if (rx_queue.empty()) {
        return {};
    }
    CAN::Message message = rx_queue.front();
    rx_queue.pop();
    return message;
};
