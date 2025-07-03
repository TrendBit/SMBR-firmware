/**
 * @file motor_transfer_function.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 03.07.2025
 */

#pragma once

#include <etl/vector.h>

#include "qlibs.h"

/**
 * @brief   Transfer function which converts speed of motor to rate and vice versa
 *          Unit of rate can be anything, but it is usually RPM, flowrate or similar
 *          Speed to normalized to range 0-1, where 0 is minimum speed and 1 is maximum speed
 *          Negative direction is not supported
 */
class Motor_transfer_function {
    /**
     * @brief Part of speed/rate table, defining speed points
     */
    etl::vector<float, 20> transfer_table_speed;

    /**
     * @brief Part of speed/rate table, defining rate points
     */
    etl::vector<float, 20> transfer_table_rate;

    /**
     * @brief   Interpolation from speed to rate
     */
    qlibs::interp1 to_rate;

    /**
     * @brief   Interpolation from rate to speed
     */
    qlibs::interp1 to_speed;

public:
    /**
     * @brief   Construct a new Motor_transfer_function object
     *          Both must have same size and be sorted in ascending order
     *
     * @param speed_table   defined speed points
     * @param rate_table    defined rate points
     */
    Motor_transfer_function(etl::vector<float, 20> speed_table, etl::vector<float, 20> rate_table) :
        transfer_table_speed(speed_table),
        transfer_table_rate(rate_table),
        to_rate(transfer_table_speed.data(), transfer_table_rate.data(),
                 transfer_table_speed.size()),
        to_speed(transfer_table_rate.data(), transfer_table_speed.data(),
                 transfer_table_speed.size())
    {
        to_speed.setMethod(qlibs::INTERP1_LINEAR);
        to_rate.setMethod(qlibs::INTERP1_LINEAR);
    }

    /**
     * @brief           Convert rate to speed
     *
     * @param rate      Rate of motor, limited to range of transfer table
     * @return float    Speed of motor in range of transfer table
     */
    float To_speed(const float rate) {
        float limited_rate = std::clamp(rate, transfer_table_rate.front(), transfer_table_rate.back());
        return to_speed.get(limited_rate);
    };

    /**
     * @brief           Convert speed to rate
     *
     * @param speed     Speed of motor, limited to range of transfer table
     * @return float    Rate of motor in range of transfer table
     */
    float To_rate(const float speed) {
        float limited_speed = std::clamp(speed, transfer_table_speed.front(), transfer_table_speed.back());
        return to_rate.get(limited_speed);
    };

    /**
     * @brief           Get maximum rate from transfer table
     *
     * @return float    Maximum rate of motor in range of transfer table
     */
    float Max_rate() const {
        return transfer_table_rate.back();
    };

    /**
     * @brief           Get minimum non-zero rate from transfer table
     *
     * @return float    Minimum rate of motor in range of transfer table
     */
    float Min_rate() const {
        auto it = std::find_if(transfer_table_rate.begin(), transfer_table_rate.end(),
                       [](float rate) { return rate > 0.0f; });
        if (it != transfer_table_rate.end()) {
            return *it;
        } else {
            return 0.0f;
        }
    };

};
