/**
 * @file enumerator.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 26.08.2025
 */

#pragma once

#define UNUSED(x) (void)(x)

#include "can_bus/message_router.hpp"
#include "can_bus/message_receiver.hpp"
#include "components/component.hpp"
#include "components/led/addressable_LED.hpp"
#include "hal/gpio/gpio_irq.hpp"
#include "logger.hpp"
#include "rtos/repeated_execution.hpp"
#include "rtos/delayed_execution.hpp"

#include "codes/codes.hpp"
#include "codes/messages/common/enumerator_collision.hpp"
#include "codes/messages/common/enumerator_reserve.hpp"

/**
 * @brief Component used for enumeration of modules and their instances in system
 *        Instance should be unique in whole system for each module.
 *        Exclusive instances are fixed and cannot be changed (only one in system should exists).
 *        Enumerated instances (instance_x) can be determined by this component from undefined instance.
 *        Instances can be mapped to colors by which modules instance can be determined by user.
 *              There are special colors:
 *                  White - Undefined instance
 *                  Red   - Enumeration issue
 */
class Enumerator: public Component, public Message_receiver {
private:
    /**
     * @brief   Instance enumeration of module, initialized by derived class calling constructor
     *          Can be changed by enumeration process if initalized as undefined
     */
    Codes::Instance current_instance;

    /**
     * @brief   Type of the module.
     */
    Codes::Module module_type;

    /**
     * @brief   Delay (ms) after enumeration process determines there are no conflicts and instance can be confirmed
     */
    uint32_t enumeration_delay_ms = 2000;

    /**
     * @brief   Delay (ms) after select mode ends and tries to reserve the selected instance
     */
    uint32_t instance_selection_delay_ms = 2000;


    /**
     * @brief   Pointer to RGB LED used for instance indication and enumeration process
     */
    Addressable_LED * enumeration_led = nullptr;

    /**
     * @brief   Pointer to button used to trigger enumeration change
     */
    GPIO_IRQ * enumeration_button = nullptr;

    /**
     * @brief   Enum representing all of the possible enumeration states
     */
    enum class State{
        selecting,
        reserving,
        in_collision,
        registered,
        exclusive
    };

    /**
     * @brief   Sets color for the different Enumerations of instance.
     */
    uint8_t colors[17][4]  = {
        { 0xff ,0x00, 0x00 }, // Undefined
        { 0xff ,0xff, 0xff }, // Exclusive
        { 0x00 ,0x00, 0x00 }, // All
        { 0xff ,0x00, 0xff }, // Reserved
        { 0xff ,0x7a, 0x00 }, // Instance_1
        { 0xff ,0x94, 0x32 }, // Instance_2
        { 0x84 ,0xff, 0x00 }, // Instance_3
        { 0x9d ,0xff, 0x32 }, // Instance_4
        { 0x00 ,0xff, 0x7a }, // Instance_5
        { 0x32 ,0xff, 0x94 }, // Instance_6
        { 0x00 ,0x84, 0xff }, // Instance_7
        { 0x32 ,0x9d, 0xff }, // Instance_8
        { 0x7a ,0x00, 0xff }, // Instance_9
        { 0x94 ,0x32, 0xff }, // Instance_10
        { 0xff ,0x00, 0x84 }, // Instance_11
        { 0xff ,0x32, 0x9d }, // Instance_12
    };
    
    /**
     * @brief   Enum containing the current enumeration state
     */
    State current_state = State::exclusive;

    /**
     * @brief   Current blinking state, true is LED on.
     */
    bool current_blinking_state = true;

    /**
     * @brief   On every other update of blinking_loop, turn the LED off.
     */
    bool do_blinking = true;

    /**
     * @brief   Holds the currently wanted_instance to be used in Change_to_instance process.
     */
    Codes::Instance wanted_instance = Codes::Instance::Undefined;

    /**
     * @brief   Repeatedly updates then LED color.
     */
    rtos::Repeated_execution *blinking_loop;

    /**
     * @brief   Starts the enumeration process for wanted_instance after a given delay.
     */
    rtos::Delayed_execution *instance_select_delay;

    /**
     * @brief   Registers the wanted_instance after a given delay.
     */
    rtos::Delayed_execution *finish_enumeration_delay;

public:
    /**
     * @brief   Construct a new Enumerator component with defined enumeration
     *              which is not intended to be changed by user (no button or indication)
     *
     * @param instance_type Instance enumeration of module
     */
    Enumerator(Codes::Module module_type ,Codes::Instance instance_type = Codes::Instance::Undefined);

    /**
     * @brief   Construct a new Enumerator component with button and RGB LED indication
     *              Button is used to trigger enumeration process (increment instance)
     *              RGB LED is used to indicate current instance by color
     *
     * @param instance_type     Instance enumeration of module
     * @param button_pin        GPIO pin number of button used to trigger enumeration process
     * @param rgb_led_pin       GPIO pin number of RGB LED used to indicate current instance by color
     */
    Enumerator(Codes::Module module_type ,Codes::Instance instance_type, uint button_pin, uint rgb_led_pin);

    /**
     * @brief  Get current instance enumeration of module
     *
     * @return Codes::Instance  Current instance enumeration
     */
    Codes::Instance Instance() const;

    /**
     * @brief   Check if current instance is stable,not in process of enumeration or undefined/error state
     *          Exclusive instance is considered always stable
     *
     * @return true     Initial enumeration was finalized
     * @return false    Stable instance is not defined, in error state or in process of enumeration
     */
    bool Stable() const;

    /**
     * @brief   Check if current instance is correct, not undefined, in error state or in process of enumeration
     *          Exclusive instance is considered correct
     *
     * @return true     Current instance is Valid
     * @return false    Current instance is undefined, in error state or still in process of enumeration
     */
    bool Valid() const;

private:
    /**
     * @brief   Load last used instance from persistent EEPROM memory
     *
     * @return Codes::Instance
     */
    Codes::Instance Load_instance_from_memory();

    /**
     * @brief   Save current instance to persistent EEPROM memory
     *
     * @return true     Instance was saved successfully
     * @return false    Instance saving failed
     */
    bool Save_instance_to_memory() const;

    /**
     * @brief   Preview current instance as color on enumeration RGB LED
     */
    void Show_instance_color() const;

    /**
     * @brief   Set color of enumeration RGB LED
     *
     * @param red   Red component of color (0-255)
     * @param green Green component of color (0-255)
     * @param blue  Blue component of color (0-255)
     */
    void Set_RGB_LED_color(uint8_t red, uint8_t green, uint8_t blue) const;

    /**
     * @brief   Start the reservation process for the wanted_instance instance.
     * 
     * @return true     Instance is being reserved
     * @return false    The reservation process is allready started and cannot be interupted. 
     */
    bool Enumerate(Codes::Instance requested_instance);

    /**
     * @brief   Register the wanted_instance as current and save it to the EEPROM memory.
     */
    void Finish_enumerate();

    /**
     * @brief   Abort the finish_enumeration_delay and set the current_instance to Undefined.
     */
    void Resolve_collision();

    /**
     * @brief   Send a collision message with the given instance.
     */
    void Send_collision_message(Codes::Instance collided_instance);

    /**
     * @brief   Method called when enumeration button is pressed by user
     *          Increments the current wanted_instance and start the Change_to_instance process.
     * @note    Won't do anything is the enumeration process is currently working.
     */
    void Enumeration_button_pressed();

    /**
     * @brief   Start the Change_to_instance process whitcj
     */
    void Change_to_instance(Codes::Instance new_instance);
protected:

    /**
     * @brief   Receive message implementation from Message_receiver interface for General/Admin messages (normal frame)
     *          This method is invoked by Message_router when message is determined for this component
     *
     * @todo    Implement all admin messages (enumeration, reset, bootloader)
     *
     * @param message   Received message
     * @return true     Message was processed by this component
     * @return false    Message cannot be processed by this component
     */
    virtual bool Receive(CAN::Message message) override final;

    /**
     * @brief   Receive message implementation from Message_receiver interface for Application messages (extended frame)
     *          This method is invoked by Message_router when message is determined for this component
     *
     * @param message   Received message
     * @return true     Message was processed by this component
     * @return false    Message cannot be processed by this component
     */
    virtual bool Receive(Application_message message) override final;

};
