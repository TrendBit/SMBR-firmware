#pragma once

#include "modules/base_module.hpp"

class Board_temperature_check {
public:
    Board_temperature_check(Base_module* module);
    void Run_check();

private:
    Base_module* module;
};
