#pragma once

/**
 * @brief Interface for all module checks
 *
 */
class IModuleCheck {

public:
    /**
     * @brief Virtual destructor for interface
     */
    virtual ~IModuleCheck() = default;

    /**
     * @brief Run the check logic
     *
     */ 
    virtual void Run_check() = 0;
};
