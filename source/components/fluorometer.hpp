/**
 * @file fluorometer.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 05.03.2025
 */

#pragma once

#include <algorithm>
#include <ranges>

#include "can_bus/app_message.hpp"
#include "can_bus/message_receiver.hpp"
#include "components/component.hpp"
#include "components/memory.hpp"
#include "components/thermometers/thermistor.hpp"
#include "components/thermometers/TMP102.hpp"
#include "hal/adc/adc_channel.hpp"
#include "hal/gpio/gpio.hpp"
#include "hal/pwm/pwm.hpp"
#include "logger.hpp"
#include "rtos/wrappers.hpp"
#include "etl/map.h"
#include "etl/vector.h"
#include "etl/array.h"



#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"

#include "codes/messages/fluorometer/fluorometer_config.hpp"
#include "codes/messages/fluorometer/ojip_capture_request.hpp"
#include "codes/messages/fluorometer/ojip_completed_response.hpp"
#include "codes/messages/fluorometer/ojip_retrieve_request.hpp"
#include "codes/messages/fluorometer/data_sample.hpp"
#include "codes/messages/fluorometer/detector_info_response.hpp"
#include "codes/messages/fluorometer/detector_temperature_response.hpp"
#include "codes/messages/fluorometer/emitor_info_response.hpp"
#include "codes/messages/fluorometer/emitor_temperature_response.hpp"

#include "codes/messages/fluorometer/sample_request.hpp"
#include "codes/messages/fluorometer/sample_response.hpp"

#define FLUOROMETER_MAX_SAMPLES 1024

typedef std::function<bool(etl::vector<uint16_t, FLUOROMETER_MAX_SAMPLES>&,uint,float)> Timing_generator_interface;

class Fluorometer: public Component, public Message_receiver {
public:

    struct OJIP{
        uint8_t measurement_id;
        float emitor_intensity;
        Fluorometer_config::Gain detector_gain;
        float sample_range;
        etl::vector<uint32_t, FLUOROMETER_MAX_SAMPLES> sample_time_us;
        etl::vector<uint16_t, FLUOROMETER_MAX_SAMPLES> intensity;
    };

    struct Calibration_data{
        std::array<uint16_t, 1000> adc_value;
        Fluorometer_config::Gain gain;
        float intensity;
        float length;
    };

private:

    /**
     * @brief   GPIO controlling input of multiplexor for temperature ADC measurement
     *              1 - onboard thermistor
     *              0 - Fluoro LED thermistor
     */
    GPIO * const ntc_channel_selector;

    /**
     * @brief   ADC channel for measuring temperature of onboard thermistor or Fluoro LED thermistor
     *          Thermistor have same values of nominal resistance and B value
     */
    Thermistor * const ntc_thermistors;

    /**
     * @brief   PWM channel for controlling main source (high-power) emitor LED of fluorometer
     */
    PWM_channel * const led_pwm;

    /**
     * @brief   Pin which controls gain of detector
     */
    GPIO * const detector_gain;

    /**
     * @brief   ADC input channel to which is connected detector output
     */
    const uint adc_input_channel = 1;

    /**
     * @brief   Slice of timer PWM which triggers sampling (whole slice isused)
     */
    const uint sampler_trigger_slice = 4;

    /**
     * @brief   Data of last measured OJIP
     *          Due to size of this 24kB+ only one instance is allowed (->static)
     */
    inline static OJIP OJIP_data = {};

    /**
     * @brief   Vector holding capture times of OJIP curve samples as micro seconds delays between captures
     *          Capture at 1ms, 5ms 15ms -> 1, 4, 10
     */
    inline static etl::vector<uint16_t, FLUOROMETER_MAX_SAMPLES> capture_timing;

    /**
     * @brief   ADC channel for measuring detector output, used only for single samples
     */
    ADC_channel * const detector_adc;

    /**
     * @brief   Flag indicating that OJIP curve capture is finished, used to wait for results by API functions
     */
    bool ojip_capture_finished = true;

    /**
     * @brief Calibration data for OJIP curve
     */
    inline static Calibration_data calibration_data = {};

    /**
     * @brief   Temperature sensor next to IR detector
     */
    TMP102 * const detector_temperature_sensor;

    /**
     * @brief   Persistent memory for OJIP calibration data
     */
    EEPROM_storage * const memory;

public:
    /**
     * @brief Construct a new Fluorometer object
     *
     * @param led_pwm               PWM channel for controlling main source (high-power) LED of fluorometer
     * @param detector_gain_pin     Pin which controls gain of detector
     * @param ntc_channel_selector  GPIO controlling input of multiplexor for temperature ADC measurement
     * @param ntc_thermistors       ADC channel for measuring temperature of onboard thermistor or Fluoro LED thermistor
     * @param i2c                   I2C bus for temp sensor
     * @param memory                EEPROM storage for calibration data
     */
    Fluorometer(PWM_channel * led_pwm, uint detector_gain_pin, GPIO * ntc_channel_selector, Thermistor * ntc_thermistors, I2C_bus * const i2c, EEPROM_storage * const memory);

    /**
     * @brief
     *
     * @param gain
     * @param emitor_intensity
     * @param capture_length
     * @param samples
     * @param timing
     * @return true
     * @return false
     */
    bool Capture_OJIP(Fluorometer_config::Gain gain, float emitor_intensity = 1.0, float capture_length = 2.0, uint samples = 1000, Fluorometer_config::Timing timing = Fluorometer_config::Timing::Linear);

    /**
     * @brief       Determines if OJIP curve capture is finished
     *              After start is false until first curve is captured
     *
     * @return true     OJIP curve capture is finished
     * @return false    OJIP curve capture is in progress, new measurement cannot be started
     */
    bool Capture_done() { return ojip_capture_finished; };

    /**
     * @brief   Retrieve captured OJIP curve
     *
     * @return OJIP*    Pointer to OJIP curve data
     */
    OJIP * Retrieve_OJIP() { return &OJIP_data; }

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
     * @brief   Read value from ADC connector to detector
     *          This should e used for single samples only is not capable of fast capture rate
     *
     * @return uint16_t     Raw value from detector
     */
    uint16_t Detector_raw_value();

    /**
     * @brief Converts existing output value of detector to range 0-1.0f
     *
     * @return float    Value of emission detector in range 0-1.0f
     */
    float Detector_value();

    /**
     * @brief  Capture value on output of detector and convert it to range 0-1.0f
     *
     * @param raw_value     Raw ADC value from detector
     * @return float        Value of emission detector in range 0-1.0f
     */
    float Detector_value(uint16_t raw_value);

    /**
     * @brief   Set intensity of main source LED of fluorometer
     *
     * @param intensity    Intensity of LED in range 0-1.0f
     */
    void Emitor_intensity(float intensity);

    /**
     * @brief   Get intensity of main source LED of fluorometer
     *
     * @return float    Intensity of LED in range 0-1.0f
     */
    float Emitor_intensity();

    /**
     * @brief   Get temperature of emitor LED
     *
     * @return float    Temperature of emitor LED in °C
     */
    float Emitor_temperature();

    /**
     * @brief   Get temperature of detector (IR PIN photodiode)
     *
     * @return float    Temperature of detector in °C
     */
    float Detector_temperature();

    /**
     * @brief Measure noise on detector output
     *
     * @param samples       Number of samples to collect
     * @param period_us     Period of sampling in microseconds
     * @return float
     */
    float Measure_noise(uint16_t samples, uint period_us);

    void Calibrate();

    /**
     * @brief       Sets gain of detector
     *
     * @param gain  Gain of detector
     */
    void Gain(Fluorometer_config::Gain gain);

    /**
     * @brief   Get current gain of detector
     *
     * @return Fluorometer_config::Gain   Gain of detector
     */
    Fluorometer_config::Gain Gain();

    /**
     * @brief  Print data via Logger in gnuplot friendly format
     *
     * @param data  Data to be printed
     */
    static void Print_curve_data(OJIP * data);

    static bool Timing_generator_linear(etl::vector<uint16_t, FLUOROMETER_MAX_SAMPLES> &capture_timing_us, uint samples, float capture_length);

    static bool Timing_generator_logarithmic(etl::vector<uint16_t, FLUOROMETER_MAX_SAMPLES> &capture_timing_us, uint samples, float capture_length);

    static bool Process_timestamps(uint64_t start, etl::vector<uint32_t, FLUOROMETER_MAX_SAMPLES> &sample_time_us);

    bool Export_data(OJIP * data);

    etl::map<Fluorometer_config::Timing, Timing_generator_interface, 16> timing_generators = {
        {Fluorometer_config::Timing::Linear, Timing_generator_linear},
        {Fluorometer_config::Timing::Logarithmic, Timing_generator_logarithmic}
    };
};
