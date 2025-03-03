#include "message_router.hpp"

bool Message_router::Route(CAN::Message message){
    // Process application messages
    if(message.Extended()){
        Application_message app_message = Application_message(message);

        Codes::Message_type message_type = app_message.Message_type();

        Logger::Print(emio::format("Routing message: {}", Codes::to_string(message_type)), Logger::Level::Trace);

        // Filter messages by target module if not in bypass list
        auto bypass = bypass_routing_table.find(message_type);

        if (bypass == bypass_routing_table.end()) {
            // Check if current module is target module
            Codes::Module target_module = app_message.Module_type();
            if (target_module != Codes::Module::All and target_module != Codes::Module::Any and target_module != Base_module::Module_type()){
                Logger::Print("Message for different module", Logger::Level::Trace);
                return false;
            } else {
                if (target_module == Codes::Module::Undefined) {
                    Logger::Print("Undefined module type", Logger::Level::Warning);
                }
            }

            // Check if current instance is target instance
            Codes::Instance target_instance = app_message.Instance_enumeration();
            if (target_instance != Codes::Instance::All and target_instance != Base_module::Instance_enumeration()) {
                Logger::Print("Message for different instance", Logger::Level::Trace);
                return false;
            } else {
                if (target_instance == Codes::Instance::Undefined) {
                    Logger::Print("Undefined instance of module", Logger::Level::Warning);
                }
            }
        } else {
            Message_receiver* instance = component_instances[bypass->second];
            if (instance) {
                instance->Receive(app_message);
                return true;
            } else {
                Logger::Print("Message receiver instance not found", Logger::Level::Warning);
                return false;
            }
        }

        // Find component which should receive this message
        auto receiver = Routing_table.find(message_type);
        if (receiver != Routing_table.end()){
            Codes::Component component = receiver->second;
            Message_receiver * instance = component_instances[component];
            if (instance) {
                instance->Receive(app_message);
                return true;
            } else {
                Logger::Print("Message receiver instance not found", Logger::Level::Warning);
                return false;
            }
        } else {
            Logger::Print("Message receiver component not found", Logger::Level::Warning);
            return false;
        }
    // Process admin messages
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
                Logger::Print("Command receiver not found", Logger::Level::Warning);
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
        Logger::Print("Component already registered, overwriting", Logger::Level::Warning);
        record->second = receiver;
    } else {
        component_instances[component] = receiver;
    }
}

void Message_router::Register_bypass(Codes::Message_type message_type, Codes::Component component_code) {
    bypass_routing_table.insert({message_type, component_code});
}
