#include "common_thread.hpp"
#include "modules/base_module.hpp"

Common_thread::Common_thread(CAN_thread * can_thread, EEPROM_storage * const memory):
    Thread("common_thread", 2048, 9),
    can_thread(can_thread),
    memory(memory)
{
    Logger::Debug("Common thread created");
    Start();
}

void Common_thread::Run(){
    Logger::Debug("Common thread start");

    bool memory_status = memory->Check_type(Base_module::Module_type(), Base_module::Instance_enumeration());

    if (!memory_status) {
        Logger::Error("Memory type check failed");
    } else {
        Logger::Debug("Memory type check passed");
    }

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1));

        while(can_thread->Message_available()) {
            auto message_in = can_thread->Read_message();
            if (message_in.has_value()) {
                Message_router::Route(message_in.value());
            }
        }
    }
}
