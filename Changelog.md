# Changelog SMPBR Firmware  
The version number consists of MAJOR.MINOR identifiers. It follows [Semantic Versioning 2.0.0](https://semver.org/) to some extent, except that it does not contain a PATCH version. Minor version changes add functionality that is backwards compatible. Major version changes may not be fully backwards compatible with the api. The file format is loosely based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).  

# (0.9) WIP (system_check)
- Update mixer fan handling at low speeds
  - Precision of RPM tracking is lower
  - Noise created by mixing fan is now removed
- Add version readout messages
  - Firmware and hardware version can be determined by CAN message
  - Also firmware commit hash, dirty flag and hardware revision is available
- Add message to read info about mixer, aerator and cuvette pump
  - Info contaning minimal and maximal supported speeds of specific motor/pump
- Update pico-sdk to version 2.2.0
  - MCU frequency is increased to 200 Mhz
- Verify correctness of UID before executing reboot or bootloader entry (can or USB)
  - There is secret key which can be used for any module
- Add Qlibs-cpp library for filter and regulators

# 0.8 (measurement_regulation)
- Add parallel processing for non-dependant messages
  - Higher bandwidth for of quick messages
  - Lower processing time for long messages like fluorometer or spectrophotometer measurements
  - Special threads are used for mutually exclusion resources (cuvette, adc)
- Add delayed repeated processing for failed measurements instead of dropping request
  - For example in case when ADC cannot measure MCU temperature because is used for OJIP sampling
- Add PI control for heater, which can hold temperature more precisely with minimal oscillations
- Add proportional regulation for mixer
  - This leads is more precise speed at higher RPM
- Add persistent calibration data for fluorometer and spectrophotometer
  - Saved in EEPROM memory and loader after startup
- Add configurable logging in KConfig
- Update watchdog handling to prevents unintended restarts
- Update thermopile initialization with filtered data are used for
  - Removed quick changes of measured temperature after startup
- Update handling of CAN bus queues
  - Input and output queues can increase processing priority when filling up

# 0.7 (sensor_module)
- Add custom messages for mini-oled
- Add regulation loop for heater based on sensor-module temperature reports
  - Enabled by `heater/target_temperature`
- Add bypass for message router so modules can receive messages addressed for other modules (temp receive)
- Add onboard thermistor support for sensor module
- Add control of case fan for control module
- Add compensation of heater non-linear power output
- Fix instability of sensor module when many temperature requests was received
- Update logger to use DMA, which enables in production logging without much additional delay
- Extend update script for automatic flashing

# 0.6 (pico-sdk_2.1.0)
- update pico-sdk and picotool to 2.1.0
- add `out/` folder where output binaries are generated
- update deployment script which is not performed inside docker container
- add backup and restore of original .config file during deployment
- move bootloader (katapult) build into container
- add `auto_flash.sh` script which can mass flash multiple board via USB
- add combined version of binary (`out/firmware.bin`) containing bootloader rand application code
- separate binary are now generated for katapult and application code
  - application(smpbr-fw) - `out/application.bin`
  - bootloader(katapult) - `out/bootloader.bin`

# 0.5 (thermal sensor)
- add bottle temperature sensor (thermopiles) as components to sensor_module
- add mini_display components to sensor_module
  - new thread controlling mini OLEd display on side of device showing device info
- add lvgl library for mini display rendering

# 0.4 (motor_control)
- Add CAN message support for new components
  - Heater: set/get intensity, get plate temperature, stop
  - Cuvette_pump: set/get intensity, set/get flowrate, move, prime, purge, stop
  - Aerator: set/get intensity, set/get flowrate, move, stop
  - Mixer: set/get speed, stir, stop
- Update bootloader for unified LED interface
- Add CAN message to read module (board) temperature via CAN

# 0.3 (led_illumination)  
- Update to new version of message codes with messages split into categories by component name
- Rename modules from board to modules, so that it's the same across the board
- Add method to read temperature of LED panel - `LED_get_temperature`
- Add method to read intensity of LED channel - `LED_get_intensity`
- Fix logger message which was truncated due to full queue
  - Now logger waits for queue to empty
- Add abstract Component class from which components inherits
  - Allows to find out what components are available on the module
  - Provides access to shared resources such as sending a message via CAN
- Extend deploy script to add version(tag), branch and dirty flash into version.txt file
- Fix missing/hanging response when core load was requested
- Fix `update_all_modules.py` to be case insensitive with firmware file naming

# 0.2 (bootloader)  
- Add bootloader as submodule ([katapult](https://github.com/Arksine/katapult))  
  - Add Kconfig config for bootloader, used for all SMPBR modules  
  - Add alternative linker script for firmware when is used with bootloader  
  - Add firmware Kconfig option to select bootloader offset 0/16 kB  
- Add targets to Makefile: `katapult` (compilation) and `katapult_flash` (via openocd and SWD)  
- Add new CAN commands to `Common_core` component  
  - `Device_reset`, `Device_usb_bootloader`, `Device_can_bootloader`  
- Change the structure of CAN messages that are now based on the fixed shared type from `can_codes` submodule  
  - This includes messages: `Ping`, `Core_temperature`, `Core_load`, `Probe_modules`  
- Add configurable green LED pin and optional yellow LED pin to to modules  
  - GPIO are passed from specific module to `Base_module`, which handles LEDs  
- Add prototype of component for other module then Base_module (`LED_Illumination` for `Control_module`)  
- Move `Heartbeat_thread` into `Base_module` from main function  
- Add python scripts  
- Add deploy script which will build firmware for all supported modules and will package it with update scripts  

# 0.1 Initial concept  
