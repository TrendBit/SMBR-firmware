#include "common_core.hpp"

#include "modules/base_module.hpp"

Common_core::Common_core(fra::MutexStandard * const adc_mutex):
    Component(Codes::Component::Common_core),
    Message_receiver(Codes::Component::Common_core),
    green_led(new GPIO(22, GPIO::Direction::Out)),
    adc_mutex(adc_mutex)
{
    green_led->Set(false);
    mcu_internal_temp = new RP_internal_temperature(3.30f);

    auto usage_sampler = [this](){
        Sample_core_load();
    };

    idle_thread_sampler = new rtos::Repeated_execution(usage_sampler, 2000, true);
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

        case Codes::Message_type::Board_temperature_request:
            return Board_temperature();

        case Codes::Message_type::Core_load_request:
            return Core_load();

        case Codes::Message_type::Probe_modules_request:
            return Probe_modules();

        case Codes::Message_type::Device_reset:
            return Reset_MCU();

        case Codes::Message_type::Device_usb_bootloader:
            return Enter_USB_bootloader();

        case Codes::Message_type::Device_can_bootloader:
            return Enter_CAN_bootloader();

        default:
            return false;
    }
}

bool Common_core::Ping(Application_message message){
    App_messages::Common::Ping_request ping_request;
    if (!ping_request.Interpret_data(message.data)){
        Logger::Print("Ping_request interpretation failed", Logger::Level::Error);
        return false;
    }

    uint8_t sequence_number = ping_request.sequence_number;
    Logger::Print(emio::format("Ping request, sequence number: {}", sequence_number), Logger::Level::Debug);
    App_messages::Common::Ping_response ping_response(sequence_number);
    Send_CAN_message(ping_response);
    return true;
}

bool Common_core::Core_temperature(){
    auto temp = MCU_core_temperature();
    if (not temp.has_value()) {
        Logger::Print("Core temperature not available", Logger::Level::Error);
        return false;
    }

    Logger::Print(emio::format("MCU_temp: {:05.2f}˚C", temp.value()), Logger::Level::Debug);
    auto temp_response = App_messages::Common::Core_temp_response(temp.value());

    Send_CAN_message(temp_response);
    return true;
}

bool Common_core::Board_temperature(){
    auto module_instance = Base_module::Instance();
    if(not module_instance){
        return false;
    }
    auto temp = module_instance->Board_temperature();
    if (not temp.has_value()) {
        Logger::Print("Board temperature not available", Logger::Level::Error);
        return false;
    }
    Logger::Print(emio::format("Board temperature: {:05.2f}˚C", temp.value()), Logger::Level::Debug);
    auto temp_response = App_messages::Common::Board_temp_response(temp.value());
    Send_CAN_message(temp_response);
    return true;
}

bool Common_core::Probe_modules(){
    auto uid = UID();
    auto probe_response = App_messages::Common::Probe_modules_response(uid);
    Logger::Print(emio::format("UID: {:02x} {:02x} {:02x} {:02x} {:02x} {:02x}", uid[0], uid[1], uid[2], uid[3], uid[4], uid[5]), Logger::Level::Debug);
    Send_CAN_message(probe_response);
    return true;
}

bool Common_core::Core_load(){
    float load = mcu_load;
    Logger::Print(emio::format("MCU load request: {:05.2f}%", load*100.0f), Logger::Level::Debug);
    auto load_response = App_messages::Common::Core_load_response(load);
    Send_CAN_message(load_response);
    return true;
}

std::array<uint8_t, CANBUS_UUID_LEN> Common_core::UID(){
    std::array<uint8_t, PICO_UUID_LEN> pico_uid;
    std::array<uint8_t, CANBUS_UUID_LEN> fast_hash_uid;

    pico_get_unique_board_id((pico_unique_board_id_t*)pico_uid.data());
    uint64_t hash = fasthash64(pico_uid.data(), PICO_UUID_LEN, KATAPULT_HASH_SEED);
    std::copy(reinterpret_cast<uint8_t*>(&hash), reinterpret_cast<uint8_t*>(&hash) + CANBUS_UUID_LEN, fast_hash_uid.begin());

    return fast_hash_uid;
}

std::optional<float> Common_core::MCU_core_temperature(){
    bool lock = adc_mutex->Lock(0);
    if (!lock) {
        Logger::Print("Core temp ADC mutex lock failed", Logger::Level::Warning);
        return std::nullopt;
    }
    float temp = mcu_internal_temp->Temperature();
    adc_mutex->Unlock();
    return temp;
}

float Common_core::MCU_core_utilization(){
    // Get the handle of the IDLE task
    TaskHandle_t idle_task = xTaskGetIdleTaskHandle();

    // Get the runtime counter for the IDLE task
    TaskStatus_t task_status;
    vTaskGetInfo(idle_task, &task_status, pdFALSE, eInvalid);

    // Get the total runtime
    uint32_t total_runtime = xTaskGetTickCount();
    uint32_t thread_runtime = task_status.ulRunTimeCounter;

    // Calculate the CPU load
    float cpuLoad = 100.0f * (1.0f - ((float)thread_runtime / (float)total_runtime));

    // Print the CPU load
    Logger::Print(emio::format("Total runtime: {} ticks", total_runtime), Logger::Level::Debug);
    Logger::Print(emio::format("Thread runtime: {} ticks", thread_runtime), Logger::Level::Debug);
    Logger::Print(emio::format("CPU load: {:05.2f}%", cpuLoad), Logger::Level::Debug);

    return cpuLoad;
}

bool Common_core::Enter_USB_bootloader(){
    watchdog_disable();
    reset_usb_boot(0, 0);
    return true;
}

bool Common_core::Enter_CAN_bootloader(){
    watchdog_disable();
    uint32_t *bl_vectors = (uint32_t *)(KATAPULT_BOOT_ADDRESS);
    uint64_t *req_sig = (uint64_t *)bl_vectors[0];
    asm volatile("cpsid i" ::: "memory");
    *req_sig = KATAPULT_REQUEST;
    NVIC_SystemReset();
    return true;
}

bool Common_core::Reset_MCU(){
    watchdog_enable(1, 1);  // Enable watchdog with loop time 1ms
    while (1);              // Loop until watchdog triggers reset
    return true;
}

void Common_core::Sample_core_load(){
    static uint32_t last_runtime_sample = 0;
    static uint32_t last_idle_thread_sample = 0;

    uint32_t current_runtime_sample = xTaskGetTickCount();

    TaskHandle_t idle_task = xTaskGetIdleTaskHandle();
    TaskStatus_t task_status;
    vTaskGetInfo(idle_task, &task_status, pdFALSE, eInvalid);
    uint32_t current_idle_thread_sample = task_status.ulRunTimeCounter;

    uint32_t runtime_sample_diff = current_runtime_sample - last_runtime_sample;
    uint32_t idle_thread_sample_diff = current_idle_thread_sample - last_idle_thread_sample;

    if (runtime_sample_diff > 0) {
        const float filter_alpha = 0.8f;
        float current_load = (1.0f - ((float)idle_thread_sample_diff / (float)runtime_sample_diff));

        if (last_runtime_sample == 0){
            // Initialization
            mcu_load = current_load;
        } else {
            // Update filtered value
            mcu_load = (1.0f - filter_alpha) * mcu_load + filter_alpha * current_load;
        }
    }

    // Update last samples for the next iteration
    last_runtime_sample = current_runtime_sample;
    last_idle_thread_sample = current_idle_thread_sample;
}
