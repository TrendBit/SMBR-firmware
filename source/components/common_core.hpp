/**
 * @file common_core.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 02.07.2024
 */

#pragma once

#include "can_bus/app_message.hpp"
#include "can_bus/message_receiver.hpp"
#include "hal/gpio/gpio.hpp"
#include "rtos/delayed_execution.hpp"
#include "rtos/repeated_execution.hpp"
#include "rtos/execute_until.hpp"

#include "components/component.hpp"
#include "components/common_sensors/RP_internal_temperature.hpp"

#include "codes/messages/common/ping_request.hpp"
#include "codes/messages/common/ping_response.hpp"
#include "codes/messages/common/probe_modules_response.hpp"
#include "codes/messages/common/core_temp_response.hpp"
#include "codes/messages/common/core_load_response.hpp"
#include "codes/messages/common/board_temp_response.hpp"

#include "pico/unique_id.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"

#include "thread.hpp"
#include "ticks.hpp"
#include "rtos/wrappers.hpp"

#include "fasthash.h"

#include "logger.hpp"

/**
 * @brief Address where Katapult bootloader is located in flash memory
 */
#define KATAPULT_BOOT_ADDRESS 0x10000100

/**
 * @brief Signature at the beginning of the bootloader determining that
 */
#define KATAPULT_SIGNATURE    0x21746f6f426e6143

/**
 * @brief Katapult command written to memory in order for Katapult to stay at bootloader after next reset
 */
#define KATAPULT_REQUEST      0x5984E3FA6CA1589B

/**
 * @brief Katapult command written to memory in order for Katapult to bypass and start application, default after power cycle
 */
#define KATAPULT_BYPASS       0x7b06ec45a9a8243d

/**
 * @brief   Seed for fast-hash algorithm used for hashing of PICO unique ID into device UUID
 *          Should be same for katapult and for application to keep same UUID
 */
#define KATAPULT_HASH_SEED    0xA16231A7

#define CANBUS_UUID_LEN       6
#define PICO_UUID_LEN         8

/**
 * @brief  This is component which is present in all modules and is responsible
 *              for processing of messages which must be supported by all modules
 *         Handles request like ping, mcu core temperature, load,  module discovery.
 */
class Common_core : public Component, Message_receiver {
private:
    /**
     * @brief Green on board LED used for signalization
     */
    GPIO *const green_led;

    /**
     * @brief Temperature sensor for measuring internal temperature of MCU
     */
    RP_internal_temperature *mcu_internal_temp;

    /**
     * @brief   Mutex for ADC access, this is used to prevent multiple threads accessing ADC at the same time
     *              Prevents measurement distortion due to different settings used on each channel
     */
    fra::MutexStandard * const  adc_mutex;

    /**
     * @brief   MCU load during last 5 seconds
     */
    float mcu_load = 0.0f;

    /**
     *  @brief Sampler capturing Idle thread utilization and calculating CPU load
     */
    rtos::Repeated_execution * idle_thread_sampler = nullptr;

public:
    /**
     * @brief Construct a new Common_core object
     *
     * @param adc_mutex     Mutex for ADC access, shared with other components from base module
     */
    explicit Common_core(fra::MutexStandard * const adc_mutex);

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

    /**
     * @brief   Process received ping request, send response with same sequence number
     *
     * @param message   Received ping request
     * @return true     Ping response was sent
     * @return false    Ping response cannot be sent (for example due to missing sequence number)
     */
    bool Ping(Application_message message);

    /**
     * @brief   Process received request for MCU core temperature, send response with current MCU core temperature
     *
     * @return true     Core temperature response was sent
     * @return false    Core temperature response cannot be sent
     */
    bool Core_temperature();

    /**
     * @brief   Respond to request for board temperature retrieved from module
     *
     * @return true     Response with board temperature was sent
     * @return false    Board temperature cannot be obtained
     */
    bool Board_temperature();

    /**
     * @brief   Respond to request for module discovery, send response with unique ID of module (MCU UID)
     *
     * @return true     Response with module ID was sent
     * @return false    Response with module ID cannot be sent
     */
    bool Probe_modules();

    /**
     * @brief Respond to request for MCU core load, response is based on idle tread utilization
     *
     * @return true     Response with core load was sent
     * @return false    Response with core load cannot be sent
     */
    bool Core_load();

    /**
     * @brief   Sample core load based on idle thread utilization
     *          Intended to be run os FreeRTOS Timer event
     */
    void Sample_core_load();

private:

    /**
     * @brief   Calculate unique ID of module, this is based on PICO unique ID
     *          Hashes pico uid using fash-hash and reduce it to 6 bytes in order to have same output as katapult
     *              This is made in order to distinguish modules but has same ui in bootloader and normal mode
     *
     * @return etl::array<uint8_t, CANBUS_UUID_LEN> Unique ID of module, reduced to 6 bytes (CANBUS_UUID_LEN)
     */
    std::array<uint8_t, CANBUS_UUID_LEN> UID();

    /**
     * @brief   Get current MCU core temperature
     *
     * @return float    Current MCU core temperature in Celsius
     */
    std::optional<float> MCU_core_temperature();

    /**
     * @brief   Get current MCU core load
     *
     * @return float    Current MCU core load in percentage (eq. percentage of idle task)
     */
    float MCU_core_utilization();

    /**
     * @brief   Device will enter RP2040 built-in USB bootloader
     *
     * @return true     Request is valid MCU will reset and enter bootloader
     * @return false    Request is invalid, MCU will continue operation
     */
    bool Enter_USB_bootloader();

    /**
     * @brief   Device will enter CAN bootloader (Katapult)
     *
     * @return true     Request is valid MCU will reset and enter Katapult bootloader
     * @return false    Request is invalid, MCU will continue operation
     */
    bool Enter_CAN_bootloader();

    /**
     * @brief   Reset MCU
     *
     * @return true     Request is valid MCU will reset
     * @return false    Request is invalid, MCU will continue operation
     */
    bool Reset_MCU();


};
