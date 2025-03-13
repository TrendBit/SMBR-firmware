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
#include "logger.hpp"

#include "codes/messages/spectrophotometer/channel_count_response.hpp"
#include "codes/messages/spectrophotometer/channel_info_request.hpp"
#include "codes/messages/spectrophotometer/channel_info_response.hpp"
#include "codes/messages/spectrophotometer/measurement_request.hpp"
#include "codes/messages/spectrophotometer/measurement_response.hpp"
#include "codes/messages/spectrophotometer/temperature_response.hpp"

/**
 * @brief   Spectrophotometer used to measure optical density of suspension
 *          Multispectral component with several channel for different wavelengths.
 */
class Spectrophotometer: public Component, public Message_receiver{
public:
    /**
     * @brief   Enumeration of all channels of spectrophotometer
     */
    enum class Channels: uint8_t {
        UV      = 0,
        Blue    = 1,
        Green   = 2,
        Red     = 3,
        Orange  = 4,
        IR      = 5,
    };

private:
    /**
     * @brief   Structure containing all information about channel of spectrophotometer
     */
    struct Channel {
        float central_wavelength;
        float half_sensitivity_width;
        float intensity_compensation;
        float multiplier;
        uint8_t driver_instance;
        KTD2026::Channel driver_channel;
        VEML6040::Channels sensor_channel;
        VEML6040::Exposure exposure_time;
    };

private:
    /**
     * @brief   Mapping between channel and components (emitor, detector and measurement settings)
     */
    static inline etl::unordered_map<Channels, Channel, 6> channels = {
        {Channels::UV,     {430, 10, 0.91, 1.0, 0, KTD2026::Channel::CH_1, VEML6040::Channels::White, VEML6040::Exposure::_640_ms}},
        {Channels::Blue,   {480, 10, 0.34, 1.0, 0, KTD2026::Channel::CH_2, VEML6040::Channels::Blue,  VEML6040::Exposure::_80_ms}},
        {Channels::Green,  {560, 10, 1.00, 1.0, 0, KTD2026::Channel::CH_3, VEML6040::Channels::Green, VEML6040::Exposure::_640_ms}},
        {Channels::Orange, {630, 10, 0.58, 1.0, 1, KTD2026::Channel::CH_1, VEML6040::Channels::Red,   VEML6040::Exposure::_80_ms}},
        {Channels::Red,    {675, 10, 0.36, 1.0, 1, KTD2026::Channel::CH_2, VEML6040::Channels::Red,   VEML6040::Exposure::_160_ms}},
        {Channels::IR,     {870, 10, 0.70, 1.0, 1, KTD2026::Channel::CH_3, VEML6040::Channels::White, VEML6040::Exposure::_40_ms}},
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

public:
    /**
     * @brief Construct a new Spectrophotometer object
     *
     * @param i2c   I2C bus where sensors are connected
     */
    explicit Spectrophotometer(I2C_bus &i2c);

    /**
     * @brief   Measure relative intensity of given channel at detector
     *          Sets up light source and wait for measurement to be done
     *          With parameters set in channels map
     *
     * @param channel   Channel to measure
     * @return float    Relative intensity of channel 0-1.0f
     */
    float Measure_relative(Channels channel);

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

private:
    /**
     * @brief   Perform calibration of emitor intensity for all channels
     */
    void Calibrate_channels();
};

