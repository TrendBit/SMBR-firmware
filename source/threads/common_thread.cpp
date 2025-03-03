#include "common_thread.hpp"

Common_thread::Common_thread(CAN_thread * can_thread):
    Thread("common_thread", 2048, 9),
    can_thread(can_thread)
{
    Logger::Print("Common thread created", Logger::Level::Debug);
    Start();
}

void Common_thread::Run(){
    Logger::Print("Common thread start", Logger::Level::Debug);

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1));

        while(can_thread->Message_available()) {
            Logger::Print("Message available", Logger::Level::Trace);
            auto message_in = can_thread->Read_message();
            if (message_in.has_value()) {
                Message_router::Route(message_in.value());
            }
        }
    }
}
