#include "message_router.hpp"

bool Message_router::Route(CAN::Message message){
    if(message.Extended()){
        Application_message app_message = Application_message(message);
        Codes::Module target_module = app_message.Module_type();
        if (target_module != Codes::Module::All and target_module != Codes::Module::Any and target_module != Base_module::Module_type()){
            Logger::Print("Message for different module");
            return false;
        } else {
            if (target_module == Codes::Module::Undefined) {
                Logger::Print("Warning: Undefined module type");
            }
        }

        Codes::Instance target_instance = app_message.Instance_enumeration();
        if (target_instance != Codes::Instance::All and target_instance != Base_module::Instance_enumeration()) {
            Logger::Print("Message for different instance");
            return false;
        } else {
            if (target_instance == Codes::Instance::Undefined) {
                Logger::Print("Warning: Undefined instance of module");
            }
        }

        Codes::Message_type message_type = app_message.Message_type();
        auto receiver = Routing_table.find(message_type);
        if (receiver != Routing_table.end()){
            Codes::Component component = receiver->second;
            Message_receiver * instance = component_instances[component];
            if (instance) {
                instance->Receive(app_message);
                return true;
            } else {
                Logger::Print("Message receiver not found");
                return false;
            }
        }
    } else {
        Codes::Command_admin cmd = static_cast<Codes::Command_admin>(message.ID());
        auto receiver = Admin_routing_table.find(cmd);
        if (receiver != Admin_routing_table.end()){
            Codes::Component component = receiver->second;
            Message_receiver * instance = component_instances[component];
            if (instance) {
                instance->Receive(message);
                return true;
            } else {
                Logger::Print("Command receiver not found");
                return false;
            }
        }
    }
    return false;
}

void Message_router::Register_receiver(Codes::Component component, Message_receiver * receiver){
    auto record = component_instances.find(component);
    // Overwrite existing record
    if (record != component_instances.end()){
        Logger::Print("Component already registered, overwriting");
        record->second = receiver;
    } else {
        component_instances[component] = receiver;
    }
}
