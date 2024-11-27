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

    args = parser.parse_args()

    interface = args.interface

    available_firmwares = [path.stem for path in pathlib.Path(args.directory).glob("*.bin")]
    if not available_firmwares:
        print("No firmware binaries found")
        exit

    modules = identify_modules(interface, timeout=2, verbose=False)
    if not modules:
        print("No modules found")
        exit(0)

    katapult_nodes = query_katapult_nodes(interface)
    if katapult_nodes:
        print(f"Some devices are already in bootloader mode, this cannot be flashed automatically:")
        for node in katapult_nodes:
            print(node)
    module_firmwares = {}

    for module in modules:
        module_type = module.module_name().lower()
        if module_type in (fw.lower() for fw in available_firmwares):
            module_firmwares[module] = module_type
        else:
            module_firmwares[module] = None

    if module_firmwares:
        print(f"\nIdentified modules: {len(modules)}\n")
        print(f"{'Module type':<25}{'Instance':<20}{'UID':<20}{'Firmware':<25}")
        print(f"{'-'*25}{'-'*20}{'-'*20}{'-'*25}")
        for module, firmware in module_firmwares.items():
            if firmware is None:
                firmware_name = "! No firmware found !"
            else:
                firmware_name = f"{firmware}.bin"
            print(f"{module.module_name():<25}{module.instance_name():<20}{module.uid_str():<20}{firmware_name:<25}")

    start_flashing = input("\nDo you want to proceed with flashing? [Y/n]: ").strip().lower()
    if start_flashing == 'n':
        print("Flashing aborted.")
        exit(0)

    for module, firmware in module_firmwares.items():
        if firmware is None:
            print(f"Skipping module {module} - No firmware found")
            continue
        print(f"Flashing module {module} with {firmware}.bin")
        print(f"Requesting bootloader entry...")
        if not request_bootloader(interface, module):
            print(f"Failed to enter bootloader mode for module {module}")
            continue
        asyncio.run(flash_module(interface, module.uid_str(), f"{args.directory}/{firmware}.bin"))
        print(f"Flashing of module {module} is completed")

