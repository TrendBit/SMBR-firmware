#pragma once
#include "module_check/IModuleCheck.hpp"
#include "codes/messages/module_issues/module_issue.hpp"
#include "logger.hpp"
#include <functional>
#include <optional>
#include <string>
#include <cmath>

/**
 * @brief   Generic limit check for monitored values.
 *
 * @tparam T Type of the monitored source object.
 */
template <typename T>
class Limit_check : public IModuleCheck {
public:

    /**
     * @brief Construct a new Limit_check.
     *
     * @param source        Pointer to the monitored source object.
     * @param getter        Function that extracts the monitored value from the source.
     * @param limit         Threshold value. If the monitored value exceeds this limit, an issue is generated.
     * @param issue_type    Type of issue to report when the limit is exceeded.
     * @param description   Description of this check (used in logs).
     * @param sendFn        Callback function used to send the generated issue.
     */
    Limit_check(T* source,
                        std::function<std::optional<float>(T*)> getter,
                        float limit,
                        App_messages::Module_issue::IssueType issue_type,
                        const std::string& description,
                        std::function<void(const App_messages::Module_issue::Module_issue&)> sendFn)
        : source(source),
          getter(std::move(getter)),
          limit(limit),
          issue_type(issue_type),
          description(description),
          sendFn(std::move(sendFn))
    {}

    /**
     * @brief Run the check against the current value.
     *
     */
    void Run_check() override {
        if (source == nullptr) {
            Logger::Error("{}: source is nullptr", description);
            return;
        }

        auto val = getter(source);
        if (!val.has_value() || std::isnan(val.value())) {
            Logger::Warning("{}: value not available", description);
            return;
        }

        float value = val.value();

        if (value > limit) {
            Logger::Warning("{}: High value detected ({:.2f})", description, value);
            App_messages::Module_issue::Module_issue issue(
                issue_type,
                App_messages::Module_issue::Severity::Error,
                value
            );
            sendFn(issue);
        }
    }

protected:
    T* source;
    std::function<std::optional<float>(T*)> getter;
    float limit;
    App_messages::Module_issue::IssueType issue_type;
    std::string description;
    std::function<void(const App_messages::Module_issue::Module_issue&)> sendFn;
};
