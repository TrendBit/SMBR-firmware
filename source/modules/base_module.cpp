#include "base_module.hpp"

#include "threads/common_thread.hpp"

Base_module::Base_module(Codes::Module module_type, Codes::Instance instance_type, uint green_led_pin):
Base_module(module_type, instance_type, green_led_pin, {})
{

}

Base_module::Base_module(Codes::Module module_type, Codes::Instance instance_type, uint green_led_pin, uint yellow_led_pin):
    Base_module(module_type, instance_type, green_led_pin, std::optional<GPIO * const>(new GPIO(yellow_led_pin, GPIO::Direction::Out)))
{

}

Base_module::Base_module(Codes::Module module_type, Codes::Instance instance_type, uint green_led_pin, std::optional<GPIO * const> yellow_led):
    module_type(module_type),
    can_thread(new CAN_thread()),
    common_thread(new Common_thread(can_thread)),
    common_core(new Common_core(can_thread)),
    heartbeat_thread(new Heartbeat_thread(green_led_pin,200)),
    yellow_led(yellow_led)
{
    this->instance = this;
    this->instance_enumeration = instance_type;

    #ifdef CONFIG_TEST_THREAD
        new Test_thread();
    #endif

    if (yellow_led.has_value()) {
        yellow_led.value()->Set(true);
    }
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
