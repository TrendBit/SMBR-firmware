#include "base_module.hpp"

#include "threads/common_thread.hpp"

Base_module::Base_module(Codes::Module module_type, Codes::Instance instance_type):
    module_type(module_type),
    can_thread(new CAN_thread()),
    common_thread(new Common_thread(can_thread)),
    common_core(new Common_core(can_thread))
{
    this->instance = this;
    this->instance_enumeration = instance_type;

    #ifdef CONFIG_TEST_THREAD
        new Test_thread();
    #endif
}

Codes::Module Base_module::Module_type() {
    if (Instance()) {
        return Instance()->module_type;
    } else {
        return Codes::Module::Undefined;
    }
}

Codes::Instance Base_module::Instance_enumeration() {
    if (Instance()) {
        return Instance()->instance_enumeration;
    } else {
        return Codes::Instance::Undefined;
    }
}

Base_module * Base_module::Instance(){
    return instance;
}
