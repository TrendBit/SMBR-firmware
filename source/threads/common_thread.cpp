#include "common_thread.hpp"

Common_thread::Common_thread(CAN_thread * can_thread):
    Thread("common_thread", 2048, 9),
    can_thread(can_thread)
{
    Logger::Print("Common thread created");
    Start();
}

void Common_thread::Run(){
    Logger::Print("Common thread start");

    while (true) {
        DelayUntil(fra::Ticks::MsToTicks(1));

        if (can_thread->Message_available()) {
            Logger::Print("Message available");
            //can_thread->Read_message().value();
            CAN::Message message_in = can_thread->Read_message().value();
            Message_router::Route(message_in);
        }
    }
}
