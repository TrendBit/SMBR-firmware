#pragma once
#include "modules/base_module.hpp"
#include "module_check/IModuleCheck.hpp"
#include "components/enumerator.hpp"
#include "codes/messages/module_issues/module_issue.hpp"

/**
 * @brief   Check for undefined instance enumeration.
 */
class Invalid_instance_check : public IModuleCheck {
public:
    const char* description = "Invalid_instance_check";
    /**
     * @brief Construct a new undefined_instance_check.
     *
     * @param enumerator    Pointer to a enumerator object.
     */
    Invalid_instance_check(Base_module* module ,Enumerator* enumerator)
        : module(module),
        enumerator(enumerator)
    {}

    /**
     * @brief Run the check.
     *
     */
    void Run_check() override {
        if (module == nullptr) {
            Logger::Error("{}: module is nullptr",description);
            return;
        }
        if (enumerator == nullptr) {
            Logger::Error("{}: enumerator is nullptr",description);
            return;
        }

        if (!enumerator->Valid() && enumerator->Stable()) {
            Logger::Warning("{}: Enumerated to invalid instance: {}",description, magic_enum::enum_name(enumerator->Instance()));
            App_messages::Module_issue::Module_issue message(
                App_messages::Module_issue::IssueType::InvalidInstance,
                static_cast<int16_t>(enumerator->Wanted_instance()),
                0.0f
            );
            module->Send_CAN_message(message);
        }
    }

protected:
    Base_module* module;
    Enumerator* enumerator;
};
