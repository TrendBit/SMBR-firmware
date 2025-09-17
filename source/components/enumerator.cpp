#include "enumerator.hpp"

Enumerator::Enumerator(Codes::Module module_type, Codes::Instance instance_type) :
    Component(Codes::Component::Enumerator),
    Message_receiver(Codes::Component::Enumerator),
    current_instance(instance_type),
    module_type(module_type)
{
    if (instance_type == Codes::Instance::Exclusive) {
        Logger::Notice("Enumerator initialized as Exclusive instance");
    } else if (instance_type == Codes::Instance::Undefined) {
        Logger::Notice("Enumerator initialized as Undefined instance, will attempt to enumerate");
    } else {
        Logger::Notice("Enumerator initialized as Instance {}", magic_enum::enum_name(instance_type));
    }
    // @todo Load instance from memory if available, need support in memory component first
}

Enumerator::Enumerator(Codes::Module module_type, Codes::Instance instance_type, uint button_pin, uint rgb_led_pin) :
    Enumerator(module_type,instance_type)
{
    // @todo Implement button and RGB LED support
    enumeration_led = new Addressable_LED(rgb_led_pin, PIO_machine(pio0, 0), 1);
    enumeration_button = new GPIO_IRQ(button_pin, GPIO::Direction::In);
    enumeration_button->Enable_IRQ(GPIO_IRQ::IRQ_level::Rising_edge, [this](){
        Enumeration_button_pressed();
    });
    Logger::Warning("Enumerator with button and RGB LED not implemented yet");
    UNUSED(button_pin);

    Set_RGB_LED_color(0xff, 0xff, 0xff);
}

Codes::Instance Enumerator::Instance() const{
    return current_instance;
}

bool Enumerator::Stable() const{
    if (current_instance == Codes::Instance::Exclusive){
        return true;
    }

    return false;
}

bool Enumerator::Valid() const{
    if (current_instance == Codes::Instance::Exclusive){
        return true;
    }

    if (current_instance == Codes::Instance::Undefined ||
        current_instance == Codes::Instance::All ||
        current_instance == Codes::Instance::Reserved) {
        return false;
    }

    return true;
}

void Enumerator::Set_RGB_LED_color(uint8_t red, uint8_t green, uint8_t blue) const {
    if (enumeration_led != nullptr) {
        enumeration_led->Set_all(red, green, blue);
    }
}

void Enumerator::Enumeration_button_pressed(){
    Logger::Trace("Enumeration button pressed");
    Set_RGB_LED_color(0xff, 0x00, 0x00);
}
bool Enumerator::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}
bool Enumerator::Receive(Application_message message){
    return true;
}