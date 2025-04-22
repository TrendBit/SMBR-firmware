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
    // Common core
    { Codes::Message_type::Device_reset,                               Codes::Component::Common_core        },
    { Codes::Message_type::Device_usb_bootloader,                      Codes::Component::Common_core        },
    { Codes::Message_type::Device_can_bootloader,                      Codes::Component::Common_core        },
    { Codes::Message_type::Probe_modules_request,                      Codes::Component::Common_core        },
    { Codes::Message_type::Ping_request,                               Codes::Component::Common_core        },
    //{ Codes::Message_type::Core_temperature_request,                   Codes::Component::Common_core        },
    { Codes::Message_type::Core_load_request,                          Codes::Component::Common_core        },
    //{ Codes::Message_type::Board_temperature_request,                  Codes::Component::Common_core        },
    // LED panel
    { Codes::Message_type::LED_set_intensity,                          Codes::Component::LED_panel          },
    { Codes::Message_type::LED_get_intensity_request,                  Codes::Component::LED_panel          },
    { Codes::Message_type::LED_get_temperature_request,                Codes::Component::LED_panel          },
    // Heater
    { Codes::Message_type::Heater_set_intensity,                       Codes::Component::Bottle_heater      },
    { Codes::Message_type::Heater_get_intensity_request,               Codes::Component::Bottle_heater      },
    { Codes::Message_type::Heater_set_target_temperature,              Codes::Component::Bottle_heater      },
    { Codes::Message_type::Heater_get_target_temperature_request,      Codes::Component::Bottle_heater      },
    { Codes::Message_type::Heater_get_plate_temperature_request,       Codes::Component::Bottle_heater      },
    { Codes::Message_type::Heater_turn_off,                            Codes::Component::Bottle_heater      },
    // Cuvette pump
    { Codes::Message_type::Cuvette_pump_set_speed,                     Codes::Component::Cuvette_pump       },
    { Codes::Message_type::Cuvette_pump_get_speed_request,             Codes::Component::Cuvette_pump       },
    { Codes::Message_type::Cuvette_pump_set_flowrate,                  Codes::Component::Cuvette_pump       },
    { Codes::Message_type::Cuvette_pump_get_flowrate_request,          Codes::Component::Cuvette_pump       },
    { Codes::Message_type::Cuvette_pump_move,                          Codes::Component::Cuvette_pump       },
    { Codes::Message_type::Cuvette_pump_prime,                         Codes::Component::Cuvette_pump       },
    { Codes::Message_type::Cuvette_pump_purge,                         Codes::Component::Cuvette_pump       },
    { Codes::Message_type::Cuvette_pump_stop,                          Codes::Component::Cuvette_pump       },
    // Aerator
    { Codes::Message_type::Aerator_set_speed,                          Codes::Component::Bottle_aerator     },
    { Codes::Message_type::Aerator_get_speed_request,                  Codes::Component::Bottle_aerator     },
    { Codes::Message_type::Aerator_set_flowrate,                       Codes::Component::Bottle_aerator     },
    { Codes::Message_type::Aerator_get_flowrate_request,               Codes::Component::Bottle_aerator     },
    { Codes::Message_type::Aerator_move,                               Codes::Component::Bottle_aerator     },
    { Codes::Message_type::Aerator_stop,                               Codes::Component::Bottle_aerator     },
    // Mixer
    { Codes::Message_type::Mixer_set_speed,                            Codes::Component::Bottle_mixer       },
    { Codes::Message_type::Mixer_get_speed_request,                    Codes::Component::Bottle_mixer       },
    { Codes::Message_type::Mixer_set_rpm,                              Codes::Component::Bottle_mixer       },
    { Codes::Message_type::Mixer_get_rpm_request,                      Codes::Component::Bottle_mixer       },
    { Codes::Message_type::Mixer_stir,                                 Codes::Component::Bottle_mixer       },
    { Codes::Message_type::Mixer_stop,                                 Codes::Component::Bottle_mixer       },
    // Bottle temperature
    { Codes::Message_type::Bottle_temperature_request,                 Codes::Component::Bottle_temperature },
    { Codes::Message_type::Bottle_top_measured_temperature_request,    Codes::Component::Bottle_temperature },
    { Codes::Message_type::Bottle_bottom_measured_temperature_request, Codes::Component::Bottle_temperature },
    { Codes::Message_type::Bottle_top_sensor_temperature_request,      Codes::Component::Bottle_temperature },
    { Codes::Message_type::Bottle_bottom_sensor_temperature_request,   Codes::Component::Bottle_temperature },
    // Mini OLED display
    { Codes::Message_type::Mini_OLED_clear_custom_text,                Codes::Component::Mini_OLED          },
    { Codes::Message_type::Mini_OLED_print_custom_text,                Codes::Component::Mini_OLED          },
    // Fluorometer
    { Codes::Message_type::Fluorometer_sample_request,                 Codes::Component::Fluorometer        },
    { Codes::Message_type::Fluorometer_OJIP_capture_request,           Codes::Component::Fluorometer        },
    { Codes::Message_type::Fluorometer_OJIP_completed_request,         Codes::Component::Fluorometer        },
    { Codes::Message_type::Fluorometer_OJIP_retrieve_request,          Codes::Component::Fluorometer        },
    { Codes::Message_type::Fluorometer_emitor_temperature_request,     Codes::Component::Fluorometer        },
    { Codes::Message_type::Fluorometer_detector_temperature_request,   Codes::Component::Fluorometer        },
    { Codes::Message_type::Fluorometer_detector_info_request,          Codes::Component::Fluorometer        },
    { Codes::Message_type::Fluorometer_emitor_info_request,            Codes::Component::Fluorometer        },
    { Codes::Message_type::Fluorometer_calibration_request,            Codes::Component::Fluorometer        },
    // Spectrophotometer
    { Codes::Message_type::Spectrophotometer_channel_count_request,     Codes::Component::Spectrophotometer  },
    { Codes::Message_type::Spectrophotometer_channel_info_request,      Codes::Component::Spectrophotometer  },
    { Codes::Message_type::Spectrophotometer_measurement_request,       Codes::Component::Spectrophotometer  },
    { Codes::Message_type::Spectrophotometer_temperature_request,       Codes::Component::Spectrophotometer  },
    { Codes::Message_type::Spectrophotometer_calibrate,                 Codes::Component::Spectrophotometer  },
};
