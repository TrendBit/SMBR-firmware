/**
 * @file spectrophotometer.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 11.03.2025
 */

#pragma once

#include "can_bus/app_message.hpp"
#include "can_bus/message_receiver.hpp"
#include "components/component.hpp"
#include "components/led/KTD2026.hpp"
#include "components/photodetectors/VEML6040.hpp"
#include "components/thermometers/TMP102.hpp"
#include "etl/array.h"
#include "etl/unordered_map.h"
#include "components/memory.hpp"
#include "logger.hpp"

#include "codes/messages/spectrophotometer/channel_count_response.hpp"
#include "codes/messages/spectrophotometer/channel_info_request.hpp"
#include "codes/messages/spectrophotometer/channel_info_response.hpp"
#include "codes/messages/spectrophotometer/measurement_request.hpp"
#include "codes/messages/spectrophotometer/measurement_response.hpp"
#include "codes/messages/spectrophotometer/temperature_response.hpp"

class Spectrophotometer_thread;

/**
 * @brief   Spectrophotometer used to measure optical density of suspension
 *          Multispectral component with several channel for different wavelengths.
 *          To determine relative value of measured channel and relative change in time spectrophotometer uses
 *              calibration which is saved persistently in EEPROM.
 *          This calibration should be done with empty cuvette or with cuvette containing grow medium.
 *          Spectrophotometer has own thread which performs tasks invoked by messages
 *              When message is routed into spectrometer by common core is placed into buffer.
 *              Spectrophotometer thread then processes messages in FIFO order.
 */
class Spectrophotometer: public Component, public Message_receiver{
    friend class Spectrophotometer_thread;

public:
    /**
     * @brief   Enumeration of all channels of spectrophotometer
     */
    enum class Channels: uint8_t {
        UV      = 0,
        Blue    = 1,
        Green   = 2,
        Orange  = 3,
        Red     = 4,
        IR      = 5,
    };

    struct Measurement {
        Channels channel;
        float relative_value;
        uint16_t absolute_value;
    };

private:
    /**
     * @brief   Structure containing all information about channel of spectrophotometer
     */
    struct Channel {
        float central_wavelength;           // Central wavelength of channel emitter in nm
        float half_sensitivity_width;       // Half sensitivity width of channel emitter in nm
        float emitter_intensity;            // Intensity of emitter used for measurements 0-1.0f
        float nominal_detection;            // Nominal detection of channel with empty cuvette or grow medium
        uint8_t driver_instance;            // Driver instance used for controlling emitter
        KTD2026::Channel driver_channel;    // Driver channel used for controlling emitter
        VEML6040::Channels sensor_channel;  // Channel of detector used for measuring light intensity
        VEML6040::Exposure exposure_time;   // Exposure time of sensor used for measuring light intensity
    };

private:
    /**
     * @brief   Mapping between channel and components (emitor, detector and measurement settings)
     */
    static inline etl::unordered_map<Channels, Channel, 6> channels = {
        {Channels::UV,     {430, 10, 1.00, 0.015, 0, KTD2026::Channel::CH_1, VEML6040::Channels::White,  VEML6040::Exposure::_640_ms}},
        {Channels::Blue,   {480, 10, 1.00, 0.300, 0, KTD2026::Channel::CH_2, VEML6040::Channels::Blue,  VEML6040::Exposure::_160_ms}},
        {Channels::Green,  {560, 10, 1.00, 0.020, 0, KTD2026::Channel::CH_3, VEML6040::Channels::Green, VEML6040::Exposure::_640_ms}},
        {Channels::Orange, {630, 10, 1.00, 0.280, 1, KTD2026::Channel::CH_1, VEML6040::Channels::Red,   VEML6040::Exposure::_160_ms}},
        {Channels::Red,    {675, 10, 1.00, 0.380, 1, KTD2026::Channel::CH_2, VEML6040::Channels::Red,   VEML6040::Exposure::_320_ms}},
        {Channels::IR,     {870, 10, 1.00, 0.315, 1, KTD2026::Channel::CH_3, VEML6040::Channels::White, VEML6040::Exposure::_80_ms}},
    };

    /**
     * @brief   Detector used for measuring light intensity
     */
    VEML6040 * const light_sensor;

    /**
     * @brief   Drivers used for controlling emitors
     */
    etl::array<KTD2026 * const, 2> drivers;

    /**
     * @brief   Temperature sensor for measuring temperature of emitors
     */
    TMP102 * const temperature_sensor;

    /**
     * @brief   Persistent memory for calibration data
     */
    EEPROM_storage * const memory;

    /**
     * @brief   Thread for offloading measurement tasks from common thread
     *          Task processed in this thread takes more time and does not block common thread
     */
    Spectrophotometer_thread * const spectrophotometer_thread;

    /**
     * @brief   Mutex for synchronizing access to cuvette which is shared by multiple components
     */
    fra::MutexStandard * const cuvette_mutex;

public:
    /**
     * @brief Construct a new Spectrophotometer object
     *
     * @param i2c               I2C bus where sensors are connected
     * @param memory            EEPROM storage for calibration data
     * @param cuvette_mutex     Mutex for synchronizing access to cuvette
     */
    explicit Spectrophotometer(I2C_bus &i2c, EEPROM_storage * const memory, fra::MutexStandard * cuvette_mutex);

    /**
     * @brief   Measure channel and return results as absolute and relative values
     *
     * @param channel        Channel to measure
     * @return Measurement   Measurement of channel
     */
    Measurement Measure_channel(Channels channel);

    /**
     * @brief   Measure relative intensity of given channel at detector
     *          Sets up light source and wait for measurement to be done
     *          With parameters set in channels map
     *
     * @param channel   Channel to measure
     * @return float    Relative intensity of channel 0-1.0f
     */
    float Measure_intensity(Channels channel);

    /**
     * @brief   Measure temperature of emitters
     *
     * @return float    Temperature in Celsius
     */
    float Temperature();

private:
    /**
     * @brief   Read raw value from detectors channel
     *
     * @param channel   Channel of detector
     * @return uint16_t Raw value from detector 16-bit unsigned
     */
    uint16_t Read_detector_raw(Channels channel);

    /**
     * @brief   Read relative value from detectors channel, without any light source configuration
     *
     * @param channel   Channel of detector
     * @return float    Relative value from detector 0-1.0f
     */
    float Read_detector(Channels channel);

    /**
     * @brief       Set intensity of emitor for given channel
     *
     * @param channel       Channel to set intensity
     * @param intensity     Intensity of emitor 0-1.0f
     * @return true         Intensity was set
     * @return false        Intensity cannot be set
     */
    bool Set(Channels channel, float intensity);

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
     * @brief   Calculate relative value of channel in respect to nominal intensity of channel
     *          Nominal intensity should be measure with empty cuvette or with cuvette containing grow medium
     *
     * @param channel       Measured channel
     * @param intensity     Intensity of channel
     * @return float        Relative value of channel in respect to nominal intensity
     */
    float Calculate_relative(Channels channel, float intensity);

    /**
     * @brief   Calculate absolute intensity of channel
     *          This calculation takes into account exposure time and used emitter intensity
     *
     * @param channel     Measured channel
     * @param intensity   Intensity of channel
     * @return uint16_t   Absolute value detected on emitter
     */
    uint16_t Calculate_absolute(Channels channel, float intensity);

    /**
     * @brief   Load calibration data from EEPROM
     *
     * @return true     Calibration data was loaded successfully
     * @return false    Calibration data was not loaded, memory not accessible or data not valid (empty)
     */
    bool Load_calibration();

    /**
     * @brief   Perform calibration of emitor intensity for all channels
     */
    void Calibrate_channels();
};

