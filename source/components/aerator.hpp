/**
 * @file aerator.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 18.01.2025
 */

#pragma once

#include <algorithm>

#include "can_bus/message_receiver.hpp"
#include "components/component.hpp"
#include "components/motors/dc_hbridge.hpp"
#include "rtos/delayed_execution.hpp"
#include "logger.hpp"
#include "can_bus/app_message.hpp"
#include "tools/motor_transfer_function.hpp"

#include "codes/messages/aerator/set_speed.hpp"
#include "codes/messages/aerator/set_flowrate.hpp"
#include "codes/messages/aerator/set_max_flowrate.hpp"
#include "codes/messages/aerator/get_speed_request.hpp"
#include "codes/messages/aerator/get_speed_response.hpp"
#include "codes/messages/aerator/get_flowrate_request.hpp"
#include "codes/messages/aerator/get_flowrate_response.hpp"
#include "codes/messages/aerator/move.hpp"
#include "codes/messages/aerator/stop.hpp"
#include "codes/messages/aerator/info_response.hpp"
#include "components/memory.hpp"

/**
 * @brief   Aerator is component which is used to aerate liquid in bottle
 *          Membrane air pump is used with DC motor controlled by H-bridge
 *          Movement of motor is possible only in one direction (pumping air into bottle)
 *              Otherwise liquid would be sucked into pump membrane
 */
class Aerator: public Component, public Message_receiver, private DC_HBridge {
public:
    /**
     * @brief Maximal flowrate fallback
     */
    static float const constexpr fallback_max_flowrate = 3000.0f;

private:
    /**
     * @brief Transfer function determining speed/flowrate curve (relative to max flowrate)
     */
    Motor_transfer_function motor_pump_speed_curve = Motor_transfer_function(
        {0, 0.1,  0.2,  0.3,  0.4,  0.5,  0.6,  0.7,  0.8,  0.9,  1.0},
        {0, 0.02, 0.4,  0.62, 0.7,  0.8,  0.82, 0.86, 0.92, 0.96, 1.0}
    );

    /**
     * @brief Timer function which stops pump after defined time, used for moving volumes of liquid
     */
    rtos::Delayed_execution * pump_stopper;

    /**
     * @brief   Persistent memory for calibration data
     */
    EEPROM_storage * const memory;

    /**
     * @brief Maximal flowrate of the pump
     */
    float max_flowrate;

public:
    /**
     * @brief Construct a new Aerator object
     *
     * @param gpio_in1      GPIO pin for control of first input of H-bridge, Forward
     * @param gpio_in2      GPIO pin for control of second input of H-bridge, Reverse
     * @param memory        Persistent memory for calibration data
     * @param min_speed     Minimum speed at which is pump moving of pump in range 0-1 (deprecated)
     * @param pwm_frequency Frequency of PWM signal for control of motor
     */
    Aerator(uint gpio_in1, uint gpio_in2, EEPROM_storage * const memory, float min_speed = 0.0f, float pwm_frequency = 50.0f);

    /**
     * @brief Set speed of pump
     */
    virtual void Speed(float speed) override final;

    /**
     * @brief Get speed of pump
     */
    virtual float Speed();

    /**
     * @brief   Set flowrate of pump
     *
     * @param flowrate  Flowrate of pump in ml/min, limited to max flow rate of pump
     * @return float    Flowrate of pump in ml/min
     */
    float Flowrate(float flowrate);

    /**
     * @brief   Get current flowrate of pump
     *
     * @return float    Current flowrate of pump in ml/min
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
     * @brief   Move given volume of air with maximal flowrate
     *          Only positive values of volume are allowed
     *
     * @param volume_ml Volume of air to move in ml
     * @return float    Time of pumping in seconds
     */
    float Move(float volume_ml);

    /**
     * @brief   Move given volume of air with given flowrate
     *          Only positive values of volume and flowrate are allowed
     *
     * @param volume_ml     Volume of air to move in ml
     * @param flowrate      Flowrate of pump in ml/min
     * @return float        Time of pumping in seconds
     */
    float Move(float volume_ml, float flowrate);

    /**
     * @brief   Stop pump immediately by coasting
     */
    void Stop() override final;

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
     * @brief   Minimal reliable flowrate of pump in ml/min
     *
     * @return float    Minimal reliable flowrate of pump in ml/min
     */
    float Minimal_flowrate() const {
        return motor_pump_speed_curve.Min_rate() * max_flowrate;
    };

    /**
     * @brief   Maximal reliable flowrate of pump in ml/min
     *
     * @return float    Maximal reliable flowrate of pump in ml/min
     */
    float Maximal_flowrate() const {
        return max_flowrate;
    };

    /**
     * @brief Loads maximal flowrate from memory to max_flowrate or uses fallback_max_flowrate value
     */
    void Load_max_flowrate();
};
