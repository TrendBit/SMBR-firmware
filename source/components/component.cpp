#include "component.hpp"

#include "modules/base_module.hpp"

Component::Component(Codes::Component component) : component(component) {
    available_components.push_back(component);
}

Codes::Component Component::Component_type() const{
    return component;
}

uint Component::Send_CAN_message(App_messages::Base_message &message) {
    return Base_module::Send_CAN_message(message);
}

etl::vector<Codes::Component, 256> Component::Available_components() {
    return available_components;
}
