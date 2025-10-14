#include "hardware/structs/clocks.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"

#include "pico/time.h"
#include "pico/multicore.h"

#include "FreeRTOS.h"
#include "thread.hpp"

#include "threads/can_thread.hpp"
#include "threads/usb_thread.hpp"


#include "cli.hpp"
#include "logger.hpp"

#include "config.hpp"

#ifdef CONFIG_CONTROL_MODULE
    #include "modules/control_module.hpp"
#elifdef CONFIG_SENSOR_MODULE
    #include "modules/sensor_module.hpp"
#elifdef CONFIG_PUMP_MODULE
    #include "modules/pump_module.hpp"
#endif
