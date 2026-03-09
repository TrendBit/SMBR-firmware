/**
 * @file pumps.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 12.12.2025
 */

#pragma once

#include "can_bus/message_receiver.hpp"
#include "component.hpp"
#include "logger.hpp"
#include "hal/gpio/gpio.hpp"
#include "hal/pwm/pwm.hpp"
#include "components/motors/dc_hbridge.hpp"
#include "components/common_sensors/current_sensor.hpp"
#include "rtos/execute_until.hpp"
#include "rtos/repeated_execution.hpp"
#include "rtos/delayed_execution.hpp"

#include "codes/messages/pumps/pump_count_response.hpp"
#include "codes/messages/pumps/set_speed.hpp"
#include "codes/messages/pumps/get_speed_request.hpp"
#include "codes/messages/pumps/get_speed_response.hpp"
#include "codes/messages/pumps/set_flowrate.hpp"
#include "codes/messages/pumps/get_flowrate_request.hpp"
#include "codes/messages/pumps/get_flowrate_response.hpp"
#include "codes/messages/pumps/info_request.hpp"
#include "codes/messages/pumps/info_response.hpp"
#include "codes/messages/pumps/move.hpp"
#include "codes/messages/pumps/stop.hpp"
#include "codes/messages/pumps/stop_all.hpp"
#include "codes/messages/pumps/set_max_flowrate.hpp"
#include "tools/motor_transfer_function.hpp"
#include "components/memory.hpp"

class Pump: private DC_HBridge{
public:
    /**
     * @brief Maximal flowrate fallback
     */
    static float const constexpr fallback_max_flowrate = 28.0f;

private:
    /**
     * @brief   PWM channel used to indicate pump activity
     */
    std::unique_ptr<PWM_channel> indication;

    /**
     * @brief   Current sensor measuring pump current
     */
    std::unique_ptr<Current_sensor> current_sensor;

    /**
     * @brief Transfer function determining speed/flowrate curve (relative to max flowrate)
     */
    Motor_transfer_function motor_pump_speed_curve = Motor_transfer_function(
        {0, 0.1,  0.2,  0.3,  0.4,  0.5,  0.6,  0.7,  0.8,  0.9,  1.0},
        {0, 0.05, 0.10, 0.29, 0.63, 0.75, 0.82, 0.89, 0.92, 0.96, 1.0}
    );

    /**
     * @brief Maximal flowrate of the pump
     */
    float max_flowrate;

    /**
     * @brief Timer function which stops pump after defined time, used for moving volumes of liquid
     */
    rtos::Delayed_execution *pump_stopper;

public:
    /**
     * @brief Construct a new Pump object
     *
     * @param gpio_in1          GPIO pin for control of first input of H-bridge, Forward
     * @param gpio_in2          GPIO pin for control of second input of H-bridge, Reverse
     * @param indication_pin     GPIO pin visualization of pump activity
     * @param current_sensor    Current sensor measuring pump current
     * @param max_flowrate      Maximum flowrate of pump in ml/min
     * @param min_speed         Minimum speed at which is pump moving of pump in range 0-1
     * @param pwm_frequency     Frequency of PWM signal for control of motor
     */
    Pump(uint8_t gpio_in1, uint8_t gpio_in2, uint8_t indication_pin, std::unique_ptr<Current_sensor> current_sensor, float max_flowrate, float min_speed = 0, float pwm_frequency = 20.0f);

    /**
     * @brief   Indicate pump activity by setting intensity of indication LED
     *
     * @param intensity     Intensity of indication LED from 0.0 to 1.0
     */
    void Indicate(float intensity);

    /**
     * @brief   Set speed of pump, overrides DC_HBridge Speed method to add indication
     *
     * @param speed     Speed of pump from -1.0 to 1.0
     */
    virtual void Speed(float speed) final;

    /**
     * @brief   Get current speed of pump
     *
     * @return float    Speed in range -1.0 to 1.0
     */
    virtual float Speed() final;

    /**
     * @brief   Set flowrate of pump
     *
     * @param flowrate    Flowrate of pump in ml/min
     */
    void Flowrate(float flowrate);

    /**
     * @brief   Get current flowrate of pump
     *
     * @return float    Flowrate in ml/min
     */
    float Flowrate();

    /**
     * @brief Sets maximal flowrate ml/min
     *
     * @param flowrate Maximal flowrate of pump in ml/min
     * @return flowrate in ml/min
     */
    float Set_Maximal_flowrate(float flowrate);

    /**
     * @brief   Get maximal flowrate of pump
     *
     * @return float    Maximal flowrate in ml/min
     */
    float Maximal_flowrate() const;

    /**
     * @brief   Get minimal flowrate of pump
     *
     * @return float    Minimal flowrate in ml/min
     */
    float Minimal_flowrate() const;

    /**
     * @brief   Stop the pump
     */
    void Stop();

    /**
     * @brief   Move given volume of liquid with given flowrate
     *          Stops automatically after calculated pumping time
     *
     * @param volume_ml     Volume of liquid to move in ml (negative value reverses direction)
     * @param flowrate      Flowrate of pump in ml/min
     * @return float        Time of pumping in seconds
     */
    float Move(float volume_ml, float flowrate);

    /**
     * @brief   Read current drawn by pump
     *
     * @return float    Current in Amperes
     */
    float Current();
};

class Pump_controller: public Component, public Message_receiver {
private:
    /**
     * @brief   List of pumps connected to module
     */
    etl::vector<Pump *,8> pumps;

    /**
     * @brief   Persistent memory for calibration data
     */
    EEPROM_storage * const memory;

public:
    /**
     * @brief Construct a new Pump_controller component
     *
     * @param pumps     List of pumps connected to module
     * @param memory    Persistent memory for calibration data
     */
    Pump_controller(etl::vector<Pump *,8> pumps, EEPROM_storage * const memory);

    /**
     * @brief   Number of pump connected to module, detection is based on configuration pin level
     */
    uint8_t Pump_count(){return pumps.size();};

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

private:
    /**
     * @brief   Check if pump index is valid
     *
     * @param index     Pump index (1-based)
     * @return true     Index is valid
     * @return false    Index is out of range
     */
    bool Valid_pump_index(uint8_t index) {
        return (index > 0) and (index <= Pump_count());
    }

    /**
     * @brief Loads maximal flowrate from memory for all pumps
     */
    void Load_max_flowrates();
};
