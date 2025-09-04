#include "module_check/core_load_check.hpp"
#include "codes/messages/module_issues/module_issue.hpp"
#include "logger.hpp"

Core_load_check::Core_load_check(Common_core* core)
    : core(core)
{}


void Core_load_check::Run_check() {
    if (core == nullptr) {
        Logger::Error("Core_load_check: core is nullptr");
        return;
    }

    auto load = core->Get_core_load();

    if (!load.has_value()) {
        Logger::Warning("Core_load_check: load not available");
        return;
    }

    float value = load.value();

    Logger::Debug("Core_load_check: current load = {:.2f} %", value);

    if (value > 0.9f) {
        Logger::Warning("Core_load_check: High CPU load detected ({:.2f} %)", value);

        App_messages::Module_issue::Module_issue issue(
            App_messages::Module_issue::IssueType::HighLoad,
            App_messages::Module_issue::Severity::Error,
            value
        );

        core->Send_CAN_message(issue);
    }
}
