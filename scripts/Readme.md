# Scripts and binaries  

This folder contains script which are used to deploy firmware binaries into modules.  
Scripts are using `flashtool.py` adapted from [katapult](https://github.com/Arksine/katapult) by Arksine [GPL 3.0 license]  

## Scripts  

`prepare_deployment.sh` - Builds binary firmware for all supported modules. Prepares a package of files containing the bootloader, firmware files and scripts needed to load the firmware into the modules. This package can then be uploaded to the selected device and the modules can be updated using the `update_all_modules.py` script.  

`update_all_modules.py` - Allows you to automatically update the firmware on all modules connected via CAN. Just run the script with the name of the folder where the firmware binaries are located. These are then flashed to the corresponding modules according to their name and module identification. Modules must not be in bootloader mode and must be running applications.  

`flash_module.py` - Uploads the firmware to the specified module connected via CAN. The module can be defined either by type and instance or by UUID. It can be in bootloader mode or it can run an application on it (the module will be switched to bootloader), but if the module is already in bootloader mode it can only be identified by UUID.  

`identify_modules.py` - Lists all modules available via CAN on which the application is running. Modules that are in the bootloader of the mod are not listed.  

`query_katapult_nodes.py` - Lists the UUIDs of all modules that are in the bootloader module.  

`application_codes.py` - Contains only definitions of constants and classes that are used in other scripts, These constants must be updated to match can_codes, otherwise flash errors may occur.  

`auto_single_flash.sh` - Automatically flashes the firmware via USB. The script sends the device to BOOT mode using the serial interface, waits for the USB drive to appear, copies the specified UF2 file to it, and waits for the device to finish rebooting. It performs a single flash per run. Usage: ./auto_single_flash.sh <binary.uf2>

### Firmware
It is an application firmware and bootloader that allows uploading the application firmware via CAN.

Bootloader `katapult.uf2` should be already loaded in all boards. It is included just in case it becomes corrupted or needs to be updated. When uploading new application firmware via CAN, the bootloader is not overwritten.

The application firmware is all files with the `*.bin` extension. These files are named using the name of the module for which they are intended. This naming convention must be followed-
