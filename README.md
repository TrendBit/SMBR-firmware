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

## Flashing  
Update of board firmware can be done via debugger(SWD) or via USB bootloader. Debugger method (`make flash`) required special hardware probe (BlackMagicProve, PicoProbe, etc), but also enables debugging. Bootloader method is more user friendly for non-developers.  

#### USB bootloader update  
Connect board on which you want to upgrade firmware to computer via USB-C connector on board which you want perform firmware upgrade. Locate two silver buttons on board marked with BOOT and RST labels.  
- Press and hold Bootloader (BOOT) button  
- Pres and release Reset button (RST) button  
- Release Bootloader (BOOT) button  
- Wait for up to 10s, then new flash drive named RP2040 should appear on computer  
- Copy new firmware (`firmware.u2f`) file to this drive  
- After file is copied, board will restart with new firmware running  
