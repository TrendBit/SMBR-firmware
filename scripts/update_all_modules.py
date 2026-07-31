import time
import logging
import asyncio
from application_codes import Module, module_types
from firmware_utils import Firmware, FirmwareVersion, load_firmwares_from_dir
from identify_modules import identify_modules
from check_version import check_version
from query_katapult_nodes import query_katapult_nodes
from flash_module import flash_module, request_bootloader
from control_oled import clear_oled, replace_oled_text, print_to_oled

import argparse

description_text = '''Flash all available SMBR modules with their respective firmware
Script will detect all connected modules and try to reboot them to bootloader mode.
If all modules are rebooted successfully then will continue to flash firmware to each module.
Use smart option to also check currently installed version on each module, and update only ones that do not have the specified version.
Binaries versions are based of their metadata. 
It can also use older binaries that don't have these, the wanted version will be used. File name of old binaries without metadata MUST 
be `<module_type>.bin` or `<module_type>.<anything>.bin`.
Directory containing firmware binaries must be provided as an argument.
Wanted version can be provided as a file, containing the target version, or the version itself (otherwise, /home/reactor/firmware/version.txt will be used). 
The version file has to have a line in the following format: `version: <version-string>`
'''

examples_text = '''Examples:
    python3 update_all_modules.py -s -d binaries/ -f version.txt -o
    python3 update_all_modules.py -c -d binaries
'''

if __name__ == "__main__":
    init_time=time.time()
    
    def log_time(identifier: str):
        global init_time
        print(f"INFO: [{identifier}] time taken: {time.time() - init_time}")
        init_time = time.time()

    last_segment = "parse_args"

    def print_segment(segment_name: str):
        global last_segment
        
        print("")
        log_time(last_segment)
        print("")
        print("-"*(len(segment_name)+2))
        print(f"|{segment_name}|")
        print("-"*(len(segment_name)+2))
        print("")
        last_segment = segment_name
        
    logging.getLogger().setLevel(logging.ERROR)
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description=description_text,
        epilog=examples_text,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-i', '--interface', type=str, default='can0', help='CAN interface to use, default can0')
    parser.add_argument('--timeout', type=int, default=2, help='Timeout in seconds, default 2')
    parser.add_argument('-c', '--old-compatible', action='store_true', help='Support old binaries without metadata')
    parser.add_argument('-y', '--yes', action='store_true', help='Flash without asking for confirmation')
    parser.add_argument('-l', '--verbose', action='store_true', help='Enable verbose output')
    parser.add_argument('-o', '--oled', action='store_true', help='Enable output to the mini OLED display')
    parser.add_argument('-s', '--smart', action='store_true', help="Also check module version and only update ones that don't allready have the correct version")
    parser.add_argument('-d', '--directory', default=".", type=str, help='Path to folder with firmware binaries')
    group1 = parser.add_mutually_exclusive_group(required=False)
    group1.add_argument('-v', '--version', type=str, help='Version, that all modules should have')
    group1.add_argument('-f', '--version-file', type=str, help='File containing a version, that all modules should have. See help for the specific format.')
    
        
    args = parser.parse_args()
    
    oled : bool = args.oled
    verbose : bool = args.verbose
    interface = args.interface   
    forced : bool = not args.smart
    version_file : str | None = args.version_file

    try:
        version = None
        if args.version:
            try:
                version = FirmwareVersion.from_string(args.version)
            except Exception as e:
                raise Exception(f"Given version is invalid: {e}")
        
        if version is None:
            version_file = "/home/reactor/firmware/version.txt"
            
        if version_file:
            try:
                version = FirmwareVersion.from_file(version_file)
            except Exception as e:
                raise Exception(f"Given version file is invalid: {e}")

    
        if verbose:
            print_segment("loading firmwares")

        fallback_version = None
        if args.old_compatible:
            if version:
                fallback_version = version
            else:
                fallback_version = FirmwareVersion(0,0,0)
            
    
        # initialize oled display
        if oled:
            replace_oled_text(interface,"Checking for module updates...",verbose)
        
        # load firmware files
        available_firmwares = load_firmwares_from_dir(args.directory,verbose=verbose,fallback_version=fallback_version)
        firmwares : dict[str,Firmware] = {}
        for module_name, module_firmwares in available_firmwares.items():
            
            if len(module_firmwares) > 1:
                if version is None:
                    raise Exception("more than one binary for the same module, please specify target version")
                
                best_fw : Firmware | None = None
                for firmware in module_firmwares:
                    if firmware.version == version:
                        if best_fw:
                            if not best_fw.with_metadata:
                                best_fw = firmware
                        else:
                            best_fw = firmware
                if best_fw:
                    firmwares[module_name] = best_fw
            else:
                firmwares[module_name] = module_firmwares[0]
        
        
        
        if verbose:
            print(f"loaded firmwares: \n{"\n".join(f"{module_name}: ({firmware.get_version_string()}) {firmware.file_name}" for module_name, firmware in firmwares.items())}")
            print("")
            print_segment("identifying modules")
    
        # identify installed modules
        modules = identify_modules(interface, timeout=2, verbose=verbose)
        if not modules:
            print("WARNING: No modules found")
            exit(0)    

        module_versions : dict[Module, FirmwareVersion] = {}
        if not forced:
            if verbose:
                print_segment("checking module versions")

            # check module versions
            for module in modules:        
                if verbose:
                    print(f"Checking module {module}")
        
                module_version = FirmwareVersion(0,0,0)
                try:
                    module_version = check_version(interface,module.module_type, module.instance,args.timeout, verbose)
                except Exception:
                    print(f"WARNING: {module} did not give its version, 0.0.0 will be used")
                module_versions[module] = module_version
                
                if verbose:
                    print(f"modules fw version: {module_version}")
                    print("")
        
        if verbose:
            print_segment("assigning updates to modules")
        
        # identify which modules can be updated with what files
        module_updates : dict[Module, Firmware] = {}
        if not forced:
            for module, module_version in module_versions.items():   
                if verbose:
                    print(f"Checking module {module}")
                module_name = module.module_name()
                firmware_candidate = firmwares.get(module_name)
                if firmware_candidate:
                    if firmware_candidate.version != module_version:
                        module_updates[module] = firmware_candidate
                        if verbose:
                            print(f"Update available: {module_version} --> {firmware_candidate.get_version_string()}")
                    else:
                        if verbose:
                            print("Up to date")
                else:
                    if verbose:
                        print("No firmware available for this module")
                    
                if verbose: 
                    print("")
        else:
            for module in modules:
                if verbose:
                    print(f"Checking module {module}")
                module_name = module.module_name()
                firmware_candidate =  firmwares.get(module_name)
                if firmware_candidate:
                    module_updates[module] = firmware_candidate
                    if verbose:
                        print(f"Forced update to {firmware_candidate.get_version_string()}")
                        print("")
                else:
                    if verbose:
                        print("No firmware available for this module")
                        print("")

        if verbose:
            print_segment("identifying modules allready in bootloader mode")
            
        # identify non-flashable modules
        katapult_nodes = query_katapult_nodes(interface)
        if katapult_nodes:
            for node in katapult_nodes:
                print(f"WARNING: Devices already in bootloader mode: {node}, it will not be updated")
                
        if verbose:
            print_segment("flashing modules")
    
        if module_updates:
            print(f"\nUpgradable modules: {len(module_updates)}\n")
            print(f"{'Module type':<25}{'Instance':<20}{'UID':<20}{'Change':<25}{'Firmware':<35}")
            print(f"{'-'*25}{'-'*20}{'-'*20}{'-'*25}{'-'*35}")
            for module, firmware in module_updates.items():
                if firmware is None:
                    firmware_name = "! No firmware found !"
                else:
                    firmware_name = f"{firmware.file_name}"
                
                module_version = module_versions.get(module)
                print(f"{module.module_name():<25}{module.instance_name():<20}{module.uid_str():<20}{f"{module_version if module_version else "?"} --> {firmware.get_version_string()}":<25}{firmware_name:<35}")
    
            if not args.yes:
                if oled:
                    replace_oled_text(interface,"Waiting for user confirmation...",verbose)
                while(True):
                    start_flashing = input("\nDo you want to proceed with flashing? [Y/n]: ").strip().lower()
                    if start_flashing == 'n':
                        print("Flashing aborted.")
                        if oled:
                            clear_oled(interface, verbose)
                        exit(0)
                    elif start_flashing == 'y':
                        print("Flashing...")
                        if oled:
                            clear_oled(interface, verbose)
                        break
            successfull_updates = 0
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
                            replace_oled_text(interface,f"{progress_text} ERROR - {module.module_name()} - bootloader",verbose)
                            time.sleep(5)
                        continue
                    
                    try:
                        asyncio.run(flash_module(interface, module.uid_str(), firmware.file_name))
                    except Exception:
                        print(f"ERROR: Failed to flash module {module}")
                        print("WARNING: Unable to restart the device. It is currently stuck in bootloader mode!!")
                        
                        if oled:
                            replace_oled_text(interface,f"{progress_text} ERROR - {module.module_name()} - flashing",verbose)
                            time.sleep(5)
                        continue
                        
                        
                    print(f"INFO: Flashing of module {module} is completed")
                    successfull_updates+=1
                    if verbose:
                        log_time(f"flash module {module}")
                    if oled and module.module_type == module_types["Sensor_module"]:
                        time.sleep(2) #wait before printing to oled again
                except Exception:
                    print(f"ERROR: Failed to update module {module}")
                    if oled:
                        replace_oled_text(interface,f"{progress_text} ERROR - {module.module_name()} - unknown",verbose)
                        
            if oled:
                replace_oled_text(interface,"Updates done ", verbose)
                for i in range(6):
                    print_to_oled(interface,".", verbose)
                    time.sleep(1)
                clear_oled(interface, verbose)
    
            print(f"INFO: Successfully updated {successfull_updates}/{len(module_updates)} modules")
        else:
            if oled:
                clear_oled(interface, verbose)
            
            print("INFO: No updates available")
    
        if verbose:
            log_time(last_segment)
    except Exception as e:
        print(f"ERROR: {e}")
        if oled:
            clear_oled(interface, verbose)
        exit(1)