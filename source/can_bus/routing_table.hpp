/**
 * @file routing_table.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 14.08.2024
 */

#include <unordered_map>

/**
 * @brief   Routing table for General/Admin messages (messages with 11-bit identifier)
 *          This are system menages messages, which are not intended for application layer
 *          Contains type of message and component which should receive this message
 *          STL version of unordered_map is used because ETL is not placed into flash memory
 */
inline static const std::unordered_map<Codes::Command_admin, Codes::Component> Admin_routing_table = {
    { Codes::Command_admin::Serial_probe,             Codes::Component::CAN_serial },
    { Codes::Command_admin::Serial_ID_respond,        Codes::Component::CAN_serial },
    { Codes::Command_admin::Serial_port_confirmation, Codes::Component::CAN_serial },
};

/**
 * @brief   Routing table for application messages (messages with 29-bit identifier)
 *          Contains type of message and component which should receive this message
 *          STL version of unordered_map is used because ETL is not placed into flash memory
 */
inline static const std::unordered_map<Codes::Message_type, Codes::Component> Routing_table = {
    { Codes::Message_type::Device_reset,                Codes::Component::Common_core      },
    { Codes::Message_type::Device_usb_bootloader,       Codes::Component::Common_core      },
    { Codes::Message_type::Device_can_bootloader,       Codes::Component::Common_core      },
    { Codes::Message_type::Probe_modules_request,       Codes::Component::Common_core      },
    { Codes::Message_type::Ping_request,                Codes::Component::Common_core      },
    { Codes::Message_type::Core_temperature_request,    Codes::Component::Common_core      },
    { Codes::Message_type::Core_load_request,           Codes::Component::Common_core      },
    { Codes::Message_type::LED_set_intensity,           Codes::Component::LED_panel        },
    { Codes::Message_type::LED_get_intensity_request,   Codes::Component::LED_panel        },
    { Codes::Message_type::LED_get_temperature_request, Codes::Component::LED_panel        },
};
