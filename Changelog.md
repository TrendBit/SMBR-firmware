# Changelog SMBR Firmware  
The version number consists of MAJOR.MINOR identifiers. It follows [Semantic Versioning 2.0.0](https://semver.org/) to some extent, except that it does not contain a PATCH version. Minor version changes add functionality that is backwards compatible. Major version changes may not be fully backwards compatible with the api. The file format is loosely based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).  

# 0.2 (bootloader)  
- Add bootloader as submodule ([katapult](https://github.com/Arksine/katapult))  
  - Add Kconfig config for bootloader, used for all SMBR modules  
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