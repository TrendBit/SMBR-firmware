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
            return Probe_modules();

        default:
            return false;
    }
}

bool Common_core::Ping(Application_message message){
    App_messages::Ping_request ping_request;
    if (!ping_request.Interpret_app_message(message)){
        Logger::Print("Ping_request interpretation failed");
        return false;
    }

    uint8_t sequence_number = ping_request.sequence_number;
    App_messages::Ping_response ping_response(sequence_number);

    can_thread->Send(ping_response.To_app_message());
    return true;
}

bool Common_core::Core_temperature(){
    float temp = mcu_internal_temp->Temperature();
    Logger::Print(emio::format("MCU_temp: {:05.2f}˚C", temp));
    auto temp_response = App_messages::Core_temp_response(temp);

    can_thread->Send(temp_response.To_app_message());
    return true;
}

bool Common_core::Probe_modules(){
    auto uid = UID();
    auto response = App_messages::Probe_modules_response(uid);
    can_thread->Send(response.To_app_message());
    return true;
}

etl::array<uint8_t, CANBUS_UUID_LEN> Common_core::UID(){
    etl::array<uint8_t, PICO_UUID_LEN> pico_uid;
    etl::array<uint8_t, CANBUS_UUID_LEN> fast_hash_uid;

    pico_get_unique_board_id((pico_unique_board_id_t*)pico_uid.data());
    uint64_t hash = fasthash64(pico_uid.data(), PICO_UUID_LEN, KATAPULT_HASH_SEED);
    std::copy(reinterpret_cast<uint8_t*>(&hash), reinterpret_cast<uint8_t*>(&hash) + CANBUS_UUID_LEN, fast_hash_uid.begin());

    return fast_hash_uid;
}

float Common_core::MCU_core_temperature(){
    return mcu_internal_temp->Temperature();
}


float Common_core::MCU_core_load(){
    // Get the handle of the IDLE task
    TaskHandle_t idle_task = xTaskGetIdleTaskHandle();

    // Get the runtime counter for the IDLE task
    TaskStatus_t task_status;
    vTaskGetInfo(idle_task, &task_status, pdFALSE, eInvalid);

    // Get the total runtime
    uint32_t total_runtime = xTaskGetTickCount();

    // Calculate the CPU load
    float cpuLoad = 100.0f * (1.0f - ((float)task_status.ulRunTimeCounter / (float)total_runtime));

    Logger::Print(emio::format("CPU load: {:04.2f}%", cpuLoad));

    return cpuLoad;
}
