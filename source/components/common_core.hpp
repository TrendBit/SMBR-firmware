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

#include "codes/messages/base_message.hpp"
#include "codes/messages/common/ping_request.hpp"
#include "codes/messages/common/ping_response.hpp"
#include "codes/messages/common/probe_modules_response.hpp"
#include "codes/messages/common/core_temp_response.hpp"
#include "codes/messages/common/core_load_response.hpp"
#include "codes/messages/common/board_temp_response.hpp"
#include "codes/messages/common/fw_version_response.hpp"
#include "codes/messages/common/fw_hash_response.hpp"
#include "codes/messages/common/fw_dirty_response.hpp"
#include "codes/messages/common/hw_version_response.hpp"

#include "pico/unique_id.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"

#include "etl/map.h"

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

/**
 * @brief   Universal key which can be used to perform restart on bootloader entry of module
 *          This should be used when module id is not known and cannot be determined (which is very limited case)
 *          But every module should perform request if issued with his ID or this
 */
#define UNIVERSAL_CONTROL_KEY { 0xca, 0xfe, 0xca, 0xfe, 0xca, 0xfe}

/**
 * @brief   Firmware version information - Major version number
 */
#ifndef FW_VERSION_MAJOR
    #define FW_VERSION_MAJOR 0
#endif

/**
 * @brief   Firmware version information - Minor version number
 */
#ifndef FW_VERSION_MINOR
    #define FW_VERSION_MINOR 0
#endif

/**
 * @brief   Firmware version information - Patch version number
 */
#ifndef FW_VERSION_PATCH
    #define FW_VERSION_PATCH 0
#endif

/**
 * @brief   Firmware version information - Git commit hash as uint64_t
 */
#ifndef FW_GIT_COMMIT_HASH_HEX
    #define FW_GIT_COMMIT_HASH_HEX 0x0000000
#endif

#ifndef FW_GIT_DIRTY
    #define FW_GIT_DIRTY true
#endif

/**
 * @brief   Length of UUID used in CANBUS messages, based on katapult UUID
 */
#define CANBUS_UUID_LEN       6

/**
 * @brief   Length of general PICO unique ID, used for module identification, obtained form Flash memory serial number
 */
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

    /**
     * @brief  Structure holding firmware version information passed by compiler based on git tags
     */
    const struct{
        uint16_t major = FW_VERSION_MAJOR;
        uint16_t minor = FW_VERSION_MINOR;
        uint16_t patch = FW_VERSION_PATCH;
    } fw_version ;

    /**
     * @brief Structure holding hardware version information
     *
     */
    struct hw_version{
        uint16_t major = 0;
        uint16_t minor = 0;
    };

    /**
     * @brief   Mapping between voltage value on version pin and hardware version
     *          This is used to determine hardware version based on voltage on version pin
     *          Voltage is generated by voltage divider on version pin
     */
    static inline const etl::map<float, hw_version, 6> hw_versions = {
        { 3.30f, { 0, 0 } }, // Reserved
        { 1.65f, { 0, 0 } }, // Reserved
        { 0.00f, { 0, 0 } }, // Reserved
        { 3.00f, { 1, 0 } },
        { 0.30f, { 1, 1 } },
        { 1.48f, { 1, 2 } }, // Double value due to mistake on control module with additional LED
        { 1.11f, { 1, 2 } },
        { 2.18f, { 1, 3 } },
        { 0.54f, { 1, 4 } },
        { 2.87f, { 1, 5 } }
    };

    /**
     * @brief Actual commit hash of firmware as uint64_t
     */
    const uint64_t fw_hash = FW_GIT_COMMIT_HASH_HEX;

    /**
     * @brief Flag indicating if firmware repository is dirty (modified)
     */
    const bool fw_dirty = FW_GIT_DIRTY;

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

    /**
     * @brief   Respond to request for firmware version
     *
     * @return true     Response with firmware version was sent
     * @return false    Unable to respond to firmware version request
     */
    bool FW_version();

    /**
     * @brief   Respond to request for firmware hash
     *
     * @return true    Response with firmware hash was sent
     * @return false   Unable to respond to firmware hash request
     */
    bool FW_hash();

    /**
     * @brief   Respond to request for firmware dirty state
     *
     * @return true     Response with firmware dirty state was sent
     * @return false    Unable to respond to firmware dirty state request
     */
    bool FW_dirty();

    /**
     * @brief   Respond to request for hardware version of module
     *
     * @return true     Hardware version was read and response was sent
     * @return false    Unable to read hardware version or send response
     */
    bool HW_version();

    /**
     * @brief   Get current MCU core temperature
     *
     * @return float    Current MCU core temperature in Celsius
     */
    std::optional<float> MCU_core_temperature();

    /**
     * @brief Returns current core load.
     *
     * @return Optional core load (0.0–1.0, or std::nullopt if unavailable.
     */
    std::optional<float> Get_core_load() const;

private:

    /**
     * @brief   Calculate unique ID of module, this is based on PICO unique ID
     *          Hashes pico uid using fash-hash and reduce it to 6 bytes in order to have same output as katapult
     *              This is made in order to distinguish modules but has same ui in bootloader and normal mode
     *
     * @return UID_t Unique ID of module, reduced to 6 bytes (CANBUS_UUID_LEN)
     */
    UID_t UID();


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
     * @param uid
     * @return true     Request is valid MCU will reset
     * @return false    Request is invalid, MCU will continue operation
     */

    bool Reset_MCU();

    /**
     * @brief   Execute function when message contains valid UID as confirmation to perform action
     *          Actions behind this check are generally not experiment safe and require confirmation
     *          But for development purposes there is bypass code for any module command
     *
     * @tparam Func     Function type
     * @param message   Received message with UID
     * @param func      Function to execute when UID is valid
     * @return bool     Return value of function if UID is valid, false otherwise
     */
    bool Execute_when_valid_UID(const Application_message& message, std::function<bool()> func) {
        if(message.data.size() != CANBUS_UUID_LEN){
            Logger::Error("Core request UID size mismatch");
            return false;
        }

        UID_t uid_remote;
        std::copy(message.data.begin(), message.data.end(), uid_remote.begin());

        UID_t uid_local = UID();
        UID_t uid_backup = UNIVERSAL_CONTROL_KEY;

        bool local_check = std::equal(uid_local.begin(), uid_local.end(), uid_remote.begin());
        bool backup_check =  std::equal(uid_backup.begin(), uid_backup.end(), uid_remote.begin());

        if (local_check) {
            return func();
        } else if (backup_check) {
            Logger::Debug("Backup UID used as Core request confirmation");
            return func();
        } else {
            Logger::Error("Core request UID validation failed");
            return false;
        }
    }

    /**
     * @brief   Read hardware version of module
     *          This is used to determine hardware version of module, which is not same as firmware version
     *          HW version info is read from resistor divider placed on PCB and coverted based on value
     *
     * @return hw_version  Hardware version of module
     */
    hw_version Read_hw_info();
};
