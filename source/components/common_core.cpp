#include "common_core.hpp"

Common_core::Common_core(CAN_thread * can_thread):
    Message_receiver(Codes::Component::Common_core),
    can_thread(can_thread),
    green_led(new GPIO(22, GPIO::Direction::Out))
{
    green_led->Set(false);
    mcu_internal_temp = new RP_internal_temperature(3.30f);
}


bool Common_core::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Common_core::Receive(Application_message message){
    std::string command_name = "";

    switch (message.Message_type()){
        case Codes::Message_type::Ping_request:
            return Ping(message);

        case Codes::Message_type::Core_temperature_request:
            return Core_temperature();

        case Codes::Message_type::Probe_modules_request:
            return Probe_modules(message);

        default:
            return false;
    }
}

bool Common_core::Ping(Application_message message){
    if (message.data.size() != 1) {
        Logger::Print("Ping message has wrong size");
        return false;
    }
    uint8_t sequence_number = message.data[0];
    CAN::Message ping = Application_message(
        Codes::Message_type::Ping_response,{sequence_number});
    can_thread->Send(ping);
    return true;
}

bool Common_core::Core_temperature(){
    float temp = mcu_internal_temp->Temperature();
    Logger::Print(emio::format("MCU_temp: {:05.2f}˚C", temp));
    CAN::Message temp_message = CAN::Message(static_cast<uint32_t>(Codes::Message_type::Core_temperature_response), {0,0,0,0,'t','e','m','p'});
    std::array<uint8_t,4> temp_bytes = *reinterpret_cast<std::array<uint8_t,4>*>(&temp);
    for (size_t i = 0; i < 4; i++){
        temp_message.data[i] = temp_bytes[i];
    }
    can_thread->Send(temp_message);
    return true;
}

bool Common_core::Probe_modules(Application_message message){
    UNUSED(message);
    etl::vector<uint8_t, 8> uid(8);
    pico_get_unique_board_id((pico_unique_board_id_t*)uid.data());
    CAN::Message response = Application_message(Codes::Message_type::Probe_modules_response, uid);
    can_thread->Send(response);
    return true;
}
