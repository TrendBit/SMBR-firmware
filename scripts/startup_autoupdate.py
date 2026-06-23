import time
import logging
import asyncio
from application_codes import Module, module_types
from firmware_utils import Firmware, FirmwareVersion, FactoryException
from identify_modules import identify_modules
from check_version import check_version
from query_katapult_nodes import query_katapult_nodes
from flash_module import flash_module, request_bootloader
from control_oled import clear_oled, replace_oled_text, print_to_oled

import pathlib
import argparse

description_text = '''Check if any installed SMBR modules have outdated firmware and flash them with newer version if needed.
Binaries versions are based of their metadata. For older binaries that don't have these, you can provide a reference version, as an argument. 
File name of old binaries without metadata MUST be `<module_type>.bin` or `<module_type>.<anything>.bin`.
Folder containing firmware binaries must be provided as argument. The newest binary for a given module type will be used.
'''

examples_text = '''Examples:
    python3 startup_autoupdate.py -d binaries
    python3 startup_autoupdate.py -d binaries -v 0.1.8
'''

if __name__ == "__main__":
    logging.getLogger().setLevel(logging.ERROR)
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description=description_text,
        epilog=examples_text,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-i', '--interface', type=str, default='can0', help='CAN interface to use, default can0')
    parser.add_argument('--timeout', type=int, default=2, help='Timeout in seconds, default 2')
    parser.add_argument('-d', '--directory', type=str, default='.', help='Path to folder with firmware binaries')
    parser.add_argument('-v', '--version', type=str, default=None, help='Reference version for old binaries')
    parser.add_argument('-y', '--yes', action='store_true', help='Flash without asking for confirmation')
    parser.add_argument('-l', '--verbose', action='store_true', help='Enable verbose output')
    parser.add_argument('-o', '--oled', action='store_true', help='Enable output to the mini OLED display')
    
    args = parser.parse_args()
    
    oled : bool = args.oled
    verbose : bool = args.verbose
    interface = args.interface    
    
    version = None
    if args.version:
        try:
            version = FirmwareVersion.from_string(args.version)
        except FactoryException as e:
            print(f"ERROR: Given version is invalid: {e}")
            exit(1)
    
    # locate firmware files
    available_firmware_files = [path for path in pathlib.Path(args.directory).glob("*.bin")]
    if not available_firmware_files:
        print("WARNING: No firmware binaries found")
        exit(0)
    
    # initialize oled display
    if oled:
        replace_oled_text(interface,"Checking for module updates...",verbose)
    
    if verbose:
        print("-------")
        print("loading firmwares:")
        print("")
        print(f"checking files: \n{"\n".join([str(file) for file in available_firmware_files])}")
        print("")

    available_firmwares : dict[str,Firmware] = {}
    for file in available_firmware_files:
        firmware = None
        error = None
        try:
            firmware = Firmware.from_file(str(file))
        except FactoryException as e:
            error = e
            if version:
                firmware = Firmware(
                    str(file),
                    file.stem.split(".",1)[0],
                    version
                )
                
        if firmware:
            available_firmwares[firmware.module_name] = firmware
        else:
            print(f"WARNING: {file} is not a valid firmware file: {error if error else "unknown error"}")

    if verbose:
        print(f"loaded firmwares: \n{"\n".join(f"{module_name}: ({firmware.version}) {firmware.file_name}" for module_name, firmware in available_firmwares.items())}")
        print("")
        print("-------")
        print("Identifying modules:")
        print("")

    # identify installed modules
    modules = identify_modules(interface, timeout=2, verbose=verbose)
    if not modules:
        print("WARNING: No modules found")
        exit(0)
    
    if verbose:
        print("")
        print("-------")
        
    # identify non-flashable modules
    katapult_nodes = query_katapult_nodes(interface)
    if katapult_nodes:
        print("WARNING: Some devices are already in bootloader mode, this cannot be flashed automatically:")
        for node in katapult_nodes:
            print(node)
    
    if verbose:
        print("checking for updates")
        print("")
    
    # identify which modules can be updated with what files
    module_versions : dict[Module, FirmwareVersion] = {}
    module_updates : dict[Module, Firmware] = {}
    for module in modules:
        module_name = module.module_name().lower()
        
        if verbose:
            print(f"Checking module {module}")
            
        module_version = check_version(interface,module.uid_str(),args.timeout, verbose)
        module_versions[module] = module_version
        
        if verbose:
            print(f"modules fw version: {module_version}")
                
        firmware_candidate = available_firmwares.get(module_name)
        if firmware_candidate:
            if firmware_candidate.version > module_version:
                module_updates[module] = firmware_candidate
                if verbose:
                    print(f"Update available: {module_version} --> {firmware_candidate.version}")
            else:
                if verbose:
                    print("Up to date")
        else:
            if verbose:
                print("No firmware available for this module")
            

        if verbose: 
            print("")

    if verbose:
        print("")
        print("-------")

    if module_updates:
        print(f"\nUpgradable modules: {len(module_updates)}\n")
        print(f"{'Module type':<25}{'Instance':<20}{'UID':<20}{'Change':<25}{'Firmware':<35}")
        print(f"{'-'*25}{'-'*20}{'-'*20}{'-'*25}{'-'*35}")
        for module, firmware in module_updates.items():
            if firmware is None:
                firmware_name = "! No firmware found !"
            else:
                firmware_name = f"{firmware.file_name}"
            
            module_version = module_versions[module]
            print(f"{module.module_name():<25}{module.instance_name():<20}{module.uid_str():<20}{f"{module_version} --> {firmware.version}":<25}{firmware_name:<35}")

        if not args.yes:
            if oled:
                replace_oled_text(interface,"Waiting for user confirmation...",verbose)
            while(True):
                start_flashing = input("\nDo you want to proceed with flashing? [Y/n]: ").strip().lower()
                if start_flashing == 'n':
                    print("Flashing aborted.")
                    exit(0)
                elif start_flashing == 'y':
                    print("Flashing...")
                    break
    
        i = 0
        # flash firmwares
        for module, firmware in module_updates.items():
            i+=1
            progress_text = f"[{i}/{len(module_updates)}]"
            
            try:
                if firmware is None:
                    print(f"WARNING: Skipping module {module} - No firmware found")
                    continue
                if oled:
                    replace_oled_text(interface, f"{progress_text} Updating {module.module_name()}...", verbose)
                    time.sleep(0.1) #wait so modules can display the information before being flashed
                    
                print(f"INFO: Flashing module {module} with {firmware.file_name}")
                print("INFO: Requesting bootloader entry...")
                
                if not request_bootloader(interface, module):
                    print(f"ERROR: Failed to enter bootloader mode for module {module}")
                    if oled:
                        replace_oled_text(interface,f"{progress_text} ERROR - bootloader",verbose)
                        time.sleep(5)
                    continue
                
                try:
                    asyncio.run(flash_module(interface, module.uid_str(), firmware.file_name))
                except:
                    print(f"ERROR: Failed to flash module {module}")
                    print("WARNING: Unable to restart the device. It is currently stuck in bootloader mode!!")
                    
                    if oled:
                        replace_oled_text(interface,f"{progress_text} ERROR - flashing",verbose)
                        time.sleep(5)
                    continue
                    
                    
                print(f"Flashing of module {module} is completed")
                if oled and module.module_type == module_types["Sensor_module"]:
                    time.sleep(2) #wait before printing to oled again
            except:
                print(f"INFO: Failed to update module {module}")
                if oled:
                    replace_oled_text(interface,f"{progress_text} ERROR - unknown",verbose)
                    
        if oled:
            replace_oled_text(interface,"Updates done ", verbose)
            for i in range(6):
                print_to_oled(interface,".", verbose)
                time.sleep(1)
            clear_oled(interface, verbose)
    
    else:
        if oled:
            clear_oled(interface, verbose)
        
        print("INFO: No updates available")