from flashtool import *
from identify_modules import *
from query_katapult_nodes import *
from application_codes import *

import time
import pathlib
import argparse

def request_bootloader(interface, module):
    bootloader_request = Message(message_types["Device_can_bootloader"], module.module_type, module.instance)
    bootloader_request.data = module.uid

    bus = can.interface.Bus(interface, bustype='socketcan')
    bus.send(bootloader_request.can_message())

    time.sleep(1)

    katapult_nodes = query_katapult_nodes(interface)

    if module.uid_str() in katapult_nodes:
        return True
    else:
        return False

async def flash_module(interface, uuid : str, firmware_file):
    uuid = int(uuid, 16)
    fpath = pathlib.Path(firmware_file).expanduser().resolve()
    loop = asyncio.get_running_loop()
    sock = CanSocket(loop)
    await sock.run(interface, uuid, fpath, False)

def detect_uuid(module_type, module_instance, modules):
    for module in modules:
        if module.module_name() == module_type and module.instance_name() == module_instance:
            return module.uid_str()
    return 0

description_text = '''Flash firmware to SMBR module
If module is running application then module type and instance can be specified to determine which module to flash.
    Second options is to use UUID of the module, which will be then rebooted to bootloader mode.
If module is in bootloader mode then only option is to use UUID.
'''

examples_text = '''Examples:
    python3 flash_module.py -f firmware.bin -m Sensor_board -n Exclusive
    python3 flash_module.py -f firmware.bin -u 3f170e95b4c6
'''

if __name__ == "__main__":
    logging.getLogger().setLevel(logging.ERROR)
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description=description_text,
        epilog=examples_text,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-i', '--interface', type=str, default='can0', help='CAN interface to use, default can0')
    parser.add_argument('-f', '--firmware', type=str, required=True, help='Path to firmware file')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-m', '--module_type', type=str, help='Module type to flash')
    group.add_argument('-u', '--uuid', type=str, help='UUID of the module to flash')

    parser.add_argument('-n', '--instance', type=str, help='Module instance to flash (required if MODULE_TYPE is specified)')

    args = parser.parse_args()

    interface = args.interface

    if args.uuid and len(args.uuid) != 12:
        parser.error("UUID must be 12 characters long")

    if args.module_type and not args.instance:
        parser.error("--instance is required when --module_type is specified")

    if args.module_type in [ "Undefined", "All", "Any", "TestBed", "Core_device"]:
        parser.error(f"Module type {args.module_type} does not support flashing")

    if args.instance in ["Undefined", "All" ,"Reserved"]:
        parser.error(f"Module instance {args.instance} is invalid, only Exclusive or specific (Instance_1-12) instances are supported)")

    modules = identify_modules(interface, timeout=2, verbose=False)
    katapult_nodes = query_katapult_nodes(interface)

    if not args.uuid:
        # Identify module if passed via type and instance
        uuid = detect_uuid(args.module_type, args.instance, modules)
    else:
        uuid = args.uuid

    module_uuids = [module.uid_str() for module in modules]
    all_uuids = module_uuids + katapult_nodes

    # Detect if module is running application or already in bootloader mode
    if uuid in module_uuids:
        module = next(module for module in modules if module.uid_str() == uuid)
        print(f"Module identifications: {module}")
        print(f"Module is running application, requesting bootloader entry...")

        if request_bootloader(interface, module):
            print(f"Module is now in bootloader mode")
        else:
            print(f"Module failed to enter bootloader mode")
            exit(0)
    elif uuid in katapult_nodes:
        print(f"Module with UUID {uuid} is already in bootloader mode")
    elif uuid not in all_uuids:
        print(f"Module with UUID {uuid} not found")
        exit(0)

    print(f"Flashing module with firmware {args.firmware}...")

    asyncio.run(flash_module(interface, uuid, args.firmware))


