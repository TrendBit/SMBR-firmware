/**
 * @file control_module.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 14.08.2024
 */

#include "base_module.hpp"
#include "logger.hpp"
#include "codes/codes.hpp"

/**
 * @brief Control module shield used for:
 *          - Temperature control
 *          - Fan control (chassis, temp fan, mixing fan)
 *          - Mixing of suspense in bottle
 *          - LED panel control
 *          - Pumping of suspense in cuvette of sensor module
 *          - Aerating of bottle
 *          - Additional temperature sensing (for example LED panel temperature)
 *        Exclusive module (only one instance in system)
 */
class Control_module: public Base_module {
public:
    /**
     * @brief Construct a new Control_module object, calls constructor of Base_module with type of module
     *          This module should be exclusive in system (Exclusive instance)
     */
    Control_module();

    /**
     * @brief Perform setup of all components of module
     */
    virtual void Setup_components() override final;
};
