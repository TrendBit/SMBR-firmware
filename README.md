# SMBR-firmware  
Firmware repository for Smart Modular BioReactor. This firmware is used with different configuration for all HW modules of device.  

## Quick getting started  
All major operation of this repository can be done in Docker container which can be build from Dockerfile. Working Docker on cross-compiling system should be only major dependency of this repository. For HW operations like flashing or debugging, Docker daemon must have correct permissions to access HW. With the approach of using docker to compile firmware, it is required to have only three dependencies running on the guest system: make, git, docker.  

Makefile in root folder of project encapsulates all operation with docker container. No further knowledge of Docker is required. Of course everything could be done without Docker container, but dependencies must be met.  

Firmware compilation and flashing (SWD method) using docker container:  
```zsh  
git clone git@github.com:TrendBit/SMBR-firmware.git  
cd SMBR-firmware  
git submodule update --init --recursive  
make docker-build  
make menuconfig # select correct module from selection  
make  
```  

## Configuration  
Since the source codes in this repository are partially shared for all modules, it is necessary to build a firmware variant for a specific module. For this purpose, `Kconfig` is used to select the desired module type. The definition of Kconfig is stored in the `config/` folder, where the selected configuration (`.config`) is written, then based on it is generated `source/config.hpp`, according to which the firmware compilation is modified.  

To select the target module run: `make menuconfig`  

## Firmware modes
The board has several mods to which it can be switched. By changing mods, for example, the board can be repaired or the firmware can be updated. Those modes are:
- `Application mode` - in this mode the board performs its primary function, this is the default mod after power-up, unless there is a malfunction or firmware corruption.
- `CAN bootloader` - allows you to update the application firmware, for example, if it has new features or is corrupted. To enter this mode, press twice RST button in quick succession.
- `USB bootloader` - this is the lowest recovery mode, which allows to upload CAN bootloader via USB, which then allows to upload application firmware. To enter this mode you must hold the BOOT button on the board and press the RST button.

## Updating  
Update of board firmware can be done via several methods: CAN bootloader, USB bootloader or debugger(SWD).  

- CAN bootloader method can be used only if module is running and connected to CAN network. This is signaled by flashing LED on module. If possible this method is __recommended__.  
- USB bootloader could be used to recover module from soft-bricked when firmware was corrupted. But in case of some modules it requires disassembly of device to access USB port of module. This method only makes the CAN bootloader operational and the CAN bootloader method must be then used to flash the application firmware onto module itself.  
- Debugger method (`make flash_katapult`) required special hardware probe (BlackMagicProve, PicoProbe, etc), but also enables debugging. Bootloader methods are more user friendly for non-developers.  

The update package can be obtained by downloading it from repository Releases or built using a script `scripts/scripts/prepare_deployment.sh`. In addition to the application firmware, the update package also contains the CAN bootloader, update scripts and a Readme with a description of the scripts and procedures. To use the update package you need to copy it to the RaspberryPi on the device and connect via `ssh`.  

### Automatic CAN update  
This method is recommended if all modules are working as they should and you just need to update the application firmware on all of them. If a module is causing a problem and cannot be updated using this method, you must resort to the manual CAN method or fix the bootloader using the USB bootloader update  

To update run script `update_all_modules.py`, which should find all available modules and determine the firmware for them. The details of script can be viewed by opening the help using `python update_all_modules.py -h`. Mandatory script parameter is the path to the folder containing the firmware -d / --directory.  

```zsh  
> python3 update_all_modules.py -d binaries  

Identified modules: 1  

Module type              Instance            UID                 Firmware                 
------------------------------------------------------------------------------------------  
Control_module           Exclusive           92a53bb7abea        control_module.bin       

Do you want to proceed with flashing? [Y/n]:  
```  

If the found modules match the reality and the corresponding firmware in the Firmware column is correctly detected, the update process can continue. The script will sequentially reset modules to the bootloader and upload the new firmware on all detected modules. These will then boot into application mode and should be detected by the script `identify_modules.py`.  

```zsh  
> python identify_modules.py  

Identified modules: 1  

Module type              Instance            UID  
---------------------------------------------------------  
Control_module           Exclusive           92a53bb7abea  
```  

This script cannot update modules that are already in CAN bootloader mode. To do this you need to use the manual method below.  

### Manual CAN update  
Manual update using a script `flash_module.py` requires you to know the signature of the module which you want to update, that is either the type and instance or the uuid of the module. Details can be found in script help page `python flash_module.py -h`. If you don't know module signature, script `identify_modules.py` will list all available modules:  
```zsh  
> python identify_modules.py  

Identified modules: 1  

Module type              Instance            UID  
---------------------------------------------------------  
Control_module           Exclusive           92a53bb7abea  
```  


If some module is not in this list, it is possible that already is in CAN bootloader (katapult) mode, and thus cannot be identified. Such modules can be listed using the `query_katapult_nodes.py` script.  

The board can be switched to CAN bootloader mode manualy by pressing the Reset (RST) button twice in quick succession. This can be used when the module is in application mode, but does not identify itself as an available module, or does not want to automatically switch to CAN bootloader mode when flashing.  

```zsh  
> python query_katapult_nodes.py  

Found the following Katapult nodes:  
92a53bb7abea  
```  

If multiple modules are identified using this method in CAN bootloader mode (katapult node), it is not possible to reliably determine which firmware should be loaded on which board. It is then necessary to disconnect the boards and determine the uuid experimentally.  

You also need to know what firmware is to come on the module, this can usually be determined from the module name and the firmware file label.  

Once you know the uuid of the module you want to update, just run the script `flash_module.py` which will switch the module mode and update the fw.  
```zsh  
> python flash_module.py -u 92a53bb7abea -f binaries/control_module.bin  
```  

The LED on the module should blink rapidly during the flash process and upon completion the application firmware with the desired functionality should be loaded and the module should reappear in the list of available modules.  

```zsh  
> python identify_modules.py  

Identified modules: 1  

Module type              Instance            UID  
---------------------------------------------------------  
Control_module           Exclusive           92a53bb7abea  
```  

### USB bootloader update  
Connect board on which you want to upgrade firmware to computer via USB-C connector on board which you want perform firmware upgrade. Locate two silver buttons on board marked with BOOT and RST labels.  
- Press and hold Bootloader (BOOT) button  
- Press and release Reset button (RST) button  
- Release Bootloader (BOOT) button  
- Wait for up to 10s, then new flash drive named RP2040 should appear on computer  
- Copy new bootloader firmware (`katapult.u2f`) file to this drive  
- After file is copied, board will restart into CAN bootloader mode  

