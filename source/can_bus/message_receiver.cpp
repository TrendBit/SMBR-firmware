#include "can_bus/message_receiver.hpp"

#include "message_router.hpp"

Message_receiver::Message_receiver(Codes::Component component){
    Message_router::Register_receiver(component, this);
}
