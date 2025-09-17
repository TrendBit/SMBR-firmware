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
     * @brief   Pointer to RGB LED used for instance indication and enumeration process
     */
    Addressable_LED * enumeration_led = nullptr;

    /**
     * @brief   Pointer to button used to trigger enumeration change
     */
    GPIO_IRQ * enumeration_button = nullptr;

public:
    /**
     * @brief   Construct a new Enumerator component with defined enumeration
     *              which is not intended to be changed by user (no button or indication)
     *
     * @param instance_type Instance enumeration of module
     */
    Enumerator(Codes::Instance instance_type = Codes::Instance::Undefined);

    /**
     * @brief   Construct a new Enumerator component with button and RGB LED indication
     *              Button is used to trigger enumeration process (increment instance)
     *              RGB LED is used to indicate current instance by color
     *
     * @param instance_type     Instance enumeration of module
     * @param button_pin        GPIO pin number of button used to trigger enumeration process
     * @param rgb_led_pin       GPIO pin number of RGB LED used to indicate current instance by color
     */
    Enumerator(Codes::Instance instance_type, uint button_pin, uint rgb_led_pin);

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
     * @brief   Process which will check usage of initial address and confirm it / select new / set error state
     * @todo    Enumeration process needs to be specified, this is only a placeholder
     */
    void Enumerate();

    /**
     * @brief   Method called when enumeration button is pressed by user
     *          Should lead to increment of instance and new enumeration validation
     */
    void Enumeration_button_pressed();
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
