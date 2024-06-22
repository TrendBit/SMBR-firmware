/**
 * @file test_thread.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 02.06.2024
 */

#pragma once

#include "can_bus/can_bus.hpp"
#include "threads/can_thread.hpp"

#include "thread.hpp"

namespace fra = cpp_freertos;

class Test_thread : public fra::Thread {
private:
    CAN_thread * can_thread;

public:
    Test_thread(CAN_thread * can_thread);

protected:
    virtual void Run();
};

