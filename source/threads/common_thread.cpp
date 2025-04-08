#include "common_thread.hpp"
#include "modules/base_module.hpp"

Common_thread::Common_thread(CAN_thread * can_thread, EEPROM_storage * const memory):
    Thread("common_thread", 2048, 9),
    can_thread(can_thread),
    memory(memory)
{
    Logger::Print("Common thread created", Logger::Level::Debug);
    Start();
}

void Common_thread::Run(){
    Logger::Print("Common thread start", Logger::Level::Debug);

    bool memory_status = memory->Check_type(Base_module::Module_type(), Base_module::Instance_enumeration());

    if (!memory_status) {
        Logger::Print("Memory type check failed", Logger::Level::Error);
    } else {
        Logger::Print("Memory type check passed", Logger::Level::Trace);
    }

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1));

        while(can_thread->Message_available()) {
            //Logger::Print("Message available", Logger::Level::Trace);
            auto message_in = can_thread->Read_message();
            if (message_in.has_value()) {
                Message_router::Route(message_in.value());
            }
        }
    }
}
