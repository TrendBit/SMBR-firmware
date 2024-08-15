#include "control_module.hpp"

Control_module::Control_module():
    Base_module(Codes::Module::Control_board, Codes::Instance::Exclusive)
{
    Setup_components();
}

void Control_module::Setup_components(){
    Logger::Print("Control module component setup");
}
