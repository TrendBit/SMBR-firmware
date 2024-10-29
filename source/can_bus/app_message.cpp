#include "app_message.hpp"

#include "modules/base_module.hpp"

Application_message::Application_message(Codes::Module module_type, Codes::Instance instance_enumeration, Codes::Message_type message_type)
    : CAN::Message((static_cast<uint32_t>(message_type) << 16) | (static_cast<uint32_t>(module_type) << 4) | (static_cast<uint32_t>(instance_enumeration)), true, false)
{ }

Application_message::Application_message(Codes::Module module_type, Codes::Instance instance_enumeration, Codes::Message_type message_type, etl::vector<uint8_t, 8> data) :
    CAN::Message((static_cast<uint32_t>(message_type) << 16) | (static_cast<uint32_t>(module_type) << 4) | (static_cast<uint32_t>(instance_enumeration)), data, true, false)
{ }

Application_message::Application_message(Codes::Message_type message_type) :
    Application_message(Base_module::Module_type(), Base_module::Instance_enumeration(), message_type)
{ }

Application_message::Application_message(Codes::Message_type message_type, etl::vector<uint8_t, 8> data) :
    Application_message(Base_module::Module_type(), Base_module::Instance_enumeration(), message_type, data)
{ }

Application_message::Application_message(App_messages::Base_message &message) :
    Application_message(Base_module::Module_type(), Base_module::Instance_enumeration(), message.Type(), message.Export_data())
{ }

Application_message::Application_message(CAN::Message message) :
    CAN::Message(message){ }

Codes::Module Application_message::Module_type() const {
    return static_cast<Codes::Module>((id >> 4) & 0xff);
}

Codes::Instance Application_message::Instance_enumeration() const {
    return static_cast<Codes::Instance>(id & 0xf);
}

Codes::Message_type Application_message::Message_type() const {
    return static_cast<Codes::Message_type>((id >> 16) & 0xfff);
}
