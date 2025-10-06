#pragma once
#include "module_check/limit_check.hpp"
#include "components/mixer.hpp"
#include "modules/base_module.hpp"

class Mixer_rpm_check : public Limit_check<Mixer> {
public:
    explicit Mixer_rpm_check(Mixer* rpm)
        : Limit_check<Mixer>(
            rpm,
            [](Mixer* p){ return std::optional<float>(p->RPM()); },
            4000.0f,
            App_messages::Module_issue::IssueType::MixerOverRPM,
            "Mixer_rpm_check",
            [rpm](const App_messages::Module_issue::Module_issue& issue) {
                auto copy = issue;
                rpm->Send_CAN_message(copy);
            })
    {}
};
