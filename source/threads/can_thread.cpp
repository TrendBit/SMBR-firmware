#include "can_thread.hpp"
#include "can_bus/can_bus.hpp"
#include "logger.hpp"
#include "config.hpp"

CAN_thread::CAN_thread()
    : Thread("can_thread", 1000, 10),
    can_bus(CAN::Bus(5, 4, CONFIG_CANBUS_SPEED, 1))
{
    Start();
};

void CAN_thread::Run(){
    while (true) {
        // Wait for any IRQ from CAN bus peripheral
        CAN::Bus::IRQ_type irq_type = can_bus.Wait_for_any<CAN::Bus::IRQ_type>();

        if(irq_type == CAN::Bus::IRQ_type::TX){ // Message was transmitted
            if (not tx_queue.empty()) {
                Retransmit();
            }
        } else if(irq_type == CAN::Bus::IRQ_type::RX){  // Message was received
            while(can_bus.Received_queue_size() > 0){
                auto message_data = can_bus.Receive();
                if (not message_data.has_value()) {
                    Logger::Error("CAN message not found after RX IRQ");
                } else {
                    // Convert message from can2040 struct to CAN::Message
                    CAN::Message message = CAN::Message(&message_data.value());
                    Receive(message);
                }
            }
        } else if (irq_type == CAN::Bus::IRQ_type::Error){  // Error occurred
            Logger::Error("CAN Error IRQ");
        } else {
            Logger::Error("CAN Unknown IRQ");   // Incorrect IRQ type evaluated
        }
    }
};

uint CAN_thread::Send(CAN::Message const &message){
    if(tx_queue.empty()){
        if(can_bus.Transmit_available()){
            Logger::Trace("CAN bus available");
            if (can_bus.Transmit(message)){
                Logger::Trace("CAN message transmitted");
            } else {
                Logger::Warning("CAN message not transmitted");
            }
        } else {
            Logger::Debug("CAN not available, message queued, size: {}, available {}", tx_queue.size(), tx_queue.available());
            tx_queue.push(message);
        }
    } else {
        Logger::Debug("CAN queue not empty, message queued, size: {}, available {}", tx_queue.size(), tx_queue.available());
        if (tx_queue.full()) {
            Logger::Warning("CAN TX queue full, message dropped");
            return 0;
        } else {
            tx_queue.push(message);
            if (tx_queue.size() % 8 == 0) {
                static uint8_t emergency_retransmit_count = 0;
                emergency_retransmit_count++;
                Logger::Warning("CAN Emergency retransmit count: {}", (short)emergency_retransmit_count);
                Logger::Warning("CAN performing retransmit attempt");
                Retransmit();
            }
        }
    }
    return tx_queue.available();
};

uint CAN_thread::Send(App_messages::Base_message &message){
    Application_message app_message(message);
    return Send(app_message);
}

void CAN_thread::Receive(CAN::Message const &message){
    if(rx_queue.full()){
        Logger::Warning("CAN RX queue full, message dropped");
        return;
    }
    rx_queue.push(message);
    Logger::Trace("CAN message received, queue size: {}, available: {}", rx_queue.size(), rx_queue.available());
};

uint8_t CAN_thread::Retransmit(){
    uint8_t retransmitted = 0;
    while((can_bus.Transmit_available()) and (not tx_queue.empty())){
        CAN::Message message = tx_queue.front();
        uint ret = can_bus.Transmit(message);
        if (not ret) {
            Logger::Error("Transmission failed");
            break;
        }
        tx_queue.pop();
        retransmitted++;
    }
    Logger::Trace("CAN retransmitted: {}", (short)retransmitted);
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
