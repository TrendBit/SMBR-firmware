#include "test_thread.hpp"
#include "can_bus/can_message.hpp"

Test_thread::Test_thread(CAN_thread * can_thread)
    : Thread("test_thread", 2048, 1),
    can_thread(can_thread){
    Start();
};

void Test_thread::Run(){
    CAN::Message message(0x123, {0,0,0,0,0,0,0,0});
    while (true) {
        can_thread->Send(message);
        message.data[0]++;
        DelayUntil(fra::Ticks::MsToTicks(500));
    }
};
