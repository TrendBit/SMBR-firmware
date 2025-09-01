#pragma once

/**
 * @brief Interface for all system checks
 *
 */
class ISystemCheck {

public:
    /**
     * @brief Virtual destructor for interface
     */
    virtual ~ISystemCheck() = default;

    /**
     * @brief Run the check logic
     *
     */ 
    virtual void Run_check() = 0;
};
