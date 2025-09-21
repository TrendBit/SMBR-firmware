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

        auto blinking_lambda = [this](){ 
            this->current_blinking_state = !this->current_blinking_state;
            this->Show_instance_color(); 
        };
        blinking_loop = new rtos::Repeated_execution(blinking_lambda, 100, false);
        
        auto instance_select_lambda = [this](){
            this->Enumerate(this->wanted_instance);
        };
        instance_select_delay = new rtos::Delayed_execution(instance_select_lambda, 1, false);

        auto finish_enumeration_lambda = [this](){
            this->Finish_enumerate();
        };
        finish_enumeration_delay = new rtos::Delayed_execution(finish_enumeration_lambda, enumeration_delay_ms, false);

        // starts the instance_select_lambda after scheduler starts (CAN_thread is ready to send messages)
        wanted_instance =  Load_instance_from_memory();
        instance_select_delay->Execute(1);
        
    } else {
        Logger::Notice("Enumerator initialized as Instance {}", magic_enum::enum_name(instance_type));

    }


    Message_router::Register_bypass(Codes::Message_type::Enumerator_collision, Codes::Component::Enumerator);
    Message_router::Register_bypass(Codes::Message_type::Enumerator_reserve,  Codes::Component::Enumerator);
}

Enumerator::Enumerator(Codes::Module module_type, Codes::Instance instance_type, uint button_pin, uint rgb_led_pin) :
    Enumerator(module_type,instance_type)
{
    enumeration_led = new Addressable_LED(rgb_led_pin, PIO_machine(pio0, 0), 1);
    enumeration_button = new GPIO_IRQ(button_pin, GPIO::Direction::In);
    enumeration_button->Enable_IRQ(GPIO_IRQ::IRQ_level::Rising_edge, [this](){
        Enumeration_button_pressed();
    });
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

Codes::Instance Enumerator::Load_instance_from_memory(){
    // @todo implement loading from memory
    Logger::Warning("Enumerator EEPROM memory loading not implemented yet");
    return Codes::Instance::Instance_1;
}

bool Enumerator::Save_instance_to_memory() const{
    // @todo implement saving to memory
    Logger::Warning("Enumerator EEPROM memory saving not implemented yet");
    return false;
}

void Enumerator::Show_instance_color() const{
    uint8_t instance_index = static_cast<uint8_t>(current_instance);
    if (current_state == State::selecting)
    {
        instance_index = static_cast<uint8_t>(wanted_instance);
    }
    
    if((!current_blinking_state && do_blinking) || current_state == State::reserving){
        Set_RGB_LED_color(0x00,0x00,0x00);
    }else{
        Set_RGB_LED_color(colors[instance_index][0],colors[instance_index][1],colors[instance_index][2]);
    }
}

void Enumerator::Set_RGB_LED_color(uint8_t red, uint8_t green, uint8_t blue) const
{
    if (enumeration_led != nullptr) {
        enumeration_led->Set_all(red/10, green/10, blue/10);
    }
}


void Enumerator::Change_to_instance(Codes::Instance new_instance){
    current_state = State::selecting;

    instance_select_delay->Abort();
    blinking_loop->Disable();

    wanted_instance = new_instance;
    do_blinking = true;

    //set the led color instantly before blinking_loop executes.
    current_blinking_state = true;
    Show_instance_color();

    blinking_loop->Enable();
    instance_select_delay->Execute(instance_selection_delay_ms);
}



bool Enumerator::Enumerate(Codes::Instance requested_instance){
    if (current_state == State::reserving){
        return false;
    }

    current_state = State::reserving;
    blinking_loop->Enable();

    wanted_instance = requested_instance;

    Logger::Notice("Enumerator is trying to reserve the Instance {}", magic_enum::enum_name(requested_instance));

    do_blinking = false;
    
    App_messages::Common::Enumerator_reserve reserve_message(requested_instance);
    Send_CAN_message(reserve_message);

    finish_enumeration_delay->Execute();

    return true;
}

void Enumerator::Finish_enumerate(){
    current_state = State::registered;
    blinking_loop->Disable();

    Logger::Notice("Enumerator has successfully registered as Instance {}", magic_enum::enum_name(wanted_instance));
    
    do_blinking = false;
    current_instance = wanted_instance;

    Show_instance_color();

    if(!Save_instance_to_memory()){
        Logger::Error("Enumerator could not save the selected Instance into EEPROM memory");
    }
}

void Enumerator::Resolve_collision(){
    current_state = State::in_collision;
    blinking_loop->Enable();
    
    finish_enumeration_delay->Abort();
    
    Logger::Warning("Enumerator has collided with another module while trying to register as Instance {}", magic_enum::enum_name(wanted_instance));
    
    current_instance = Codes::Instance::Undefined;
    current_blinking_state = true;

}

void Enumerator::Send_collision_message(Codes::Instance collided_instance){
    App_messages::Common::Enumerator_collision collision_message(collided_instance);
    Send_CAN_message(collision_message);
}

void Enumerator::Enumeration_button_pressed(){
    Logger::Trace("Enumeration button pressed");
    if (current_state != State::reserving){
        auto incremented_instance = (static_cast<int>(wanted_instance)+1) % 16;
        if (incremented_instance < static_cast<int>(Codes::Instance::Instance_1)){
            incremented_instance = static_cast<int>(Codes::Instance::Instance_1);
        }
        Logger::Debug("User selected Instance: {}",  magic_enum::enum_name(static_cast<Codes::Instance>(incremented_instance)));
        Change_to_instance(static_cast<Codes::Instance>(incremented_instance));
    }
    
}

bool Enumerator::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Enumerator::Receive(Application_message message){
    switch (message.Message_type()){
        case Codes::Message_type::Enumerator_collision: {
            if (message.Module_type() != module_type){
                Logger::Trace("Enumerator_collision interpretation skipped, it was for a different module");
                return true;
            }
            
            // ignore if not currently reserving
            if (current_state != State::reserving){
                Logger::Trace("Enumerator_collision interpretation skipped, not able to interpret in current state");
                return true;
            }
            
            App_messages::Common::Enumerator_collision enumerator_collision;

            if (!enumerator_collision.Interpret_data(message.data)){
                Logger::Error("Enumerator_collision interpretation failed");
                return false;
            }

            Logger::Debug("Enumerator_collision message detected for instance: {}",  magic_enum::enum_name(enumerator_collision.collided_instance));
            
            if (enumerator_collision.collided_instance == wanted_instance){
                Resolve_collision();
            }

            return true;
        }

        case Codes::Message_type::Enumerator_reserve: {
            if (message.Module_type() != module_type){
                Logger::Trace("Enumerator_reserve interpretation skipped, it was for a different module");
                return true;
            }

            // ignore if not reserving or don't have a registered instance
            if (!(current_state == State::reserving || current_state == State::registered)){
                Logger::Trace("Enumerator_reserve interpretation skipped, not able to interpret in current state");
                return true;
            }

            App_messages::Common::Enumerator_reserve enumerator_reserve;

            if (!enumerator_reserve.Interpret_data(message.data)){
                Logger::Error("Enumerator_reserve interpretation failed");
                return false;
            }

            Logger::Debug("Enumerator_reserve message detected for instance: {}",  magic_enum::enum_name(enumerator_reserve.requested_instance));
            
            if (enumerator_reserve.requested_instance == wanted_instance){
                Send_collision_message(wanted_instance);

                // don't resolve the colision if having a registered instance
                if (current_state == State::reserving){
                    Resolve_collision();
                }
                
            }

            return true;
        }

        default:
            return false;
    }
    return true;
}