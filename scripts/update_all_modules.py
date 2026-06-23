from identify_modules import *
from query_katapult_nodes import *
from flash_module import *

import time
import pathlib
import argparse

description_text = '''Flash all available SMBR modules with their respective firmware
Script will detect all connected modules and try to reboot them to bootloader mode.
If all modules are rebooted successfully then will continue to flash firmware to each module.
Folder containing firmware binaries must be provided as argument. And binaries must have names corresponding to module types.
'''

examples_text = '''Examples:
    python3 update_all_modules.py -d binaries
'''

if __name__ == "__main__":
    logging.getLogger().setLevel(logging.ERROR)
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description=description_text,
        epilog=examples_text,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-i', '--interface', type=str, default='can0', help='CAN interface to use, default can0')
    parser.add_argument('-d', '--directory', type=str, default='.', help='Path to folder with firmware binaries')
    parser.add_argument('-y', '--yes', action='store_true', help='Flash without asking for confirmation')


    args = parser.parse_args()

    interface = args.interface
    
    # locate firmware files
    available_firmware_files = [path for path in pathlib.Path(args.directory).glob("*.bin")]
    if not available_firmware_files:
        print("No firmware binaries found")
        exit(0)
    
    # check firmware metadata and select the newest firmwares
    available_firmwares : dict[str,Firmware] = {}
    for file in available_firmware_files:
        try:
            firmware = Firmware.from_file(str(file))
            if available_firmwares.get(firmware.module_name):
                if available_firmwares[firmware.module_name].version > firmware.version:
                    continue
            available_firmwares[firmware.module_name] = firmware
        except FactoryException as e:
            print(f"File {file} is not a valid firmware: {e}")

    # identify modules
    modules = identify_modules(interface, timeout=2, verbose=False)
    if not modules:
        print("No modules found")
        exit(0)

    # check module bootloader states
    katapult_nodes = query_katapult_nodes(interface)
    if katapult_nodes:
        print(f"Some devices are already in bootloader mode, this cannot be flashed automatically:")
        for node in katapult_nodes:
            print(node)
            
    # pair modules with their firmwares
    module_firmwares : dict[Module,str | None] = {}
    for module in modules:
        module_type = module.module_name().lower()
        fw_candidate = available_firmwares.get(module.module_name().lower())
        if fw_candidate:
            module_firmwares[module] = str(fw_candidate.file_name)
        else:
            module_firmwares[module] = None

    if module_firmwares:
        print(f"\nIdentified modules: {len(modules)}\n")
        print(f"{'Module type':<25}{'Instance':<20}{'UID':<20}{'Firmware':<25}")
        print(f"{'-'*25}{'-'*20}{'-'*20}{'-'*25}")
        for module, firmware in module_firmwares.items():
            if firmware is None:
                firmware_file = "! No firmware found !"
            else:
                firmware_file = firmware
            print(f"{module.module_name():<25}{module.instance_name():<20}{module.uid_str():<20}{firmware_file:<25}")

    if not args.yes:
        while(True):
            start_flashing = input("\nDo you want to proceed with flashing? [Y/n]: ").strip().lower()
            if start_flashing == 'n':
                print("Flashing aborted.")
                exit(0)
            elif start_flashing == 'y':
                break

    for module, firmware in module_firmwares.items():
        if firmware is None:
            print(f"Skipping module {module} - No firmware found")
            continue
        print(f"Flashing module {module} with {firmware}")
        print(f"Requesting bootloader entry...")
        if not request_bootloader(interface, module):
            print(f"Failed to enter bootloader mode for module {module}")
            continue
        asyncio.run(flash_module(interface, module.uid_str(), firmware))
        print(f"Flashing of module {module} is completed")

