import time

import can
import argparse

from application_codes import Message, message_types, module_types, module_instances
from firmware_utils import FirmwareVersion


def check_version(can_interface, module_type: int, module_instance : int, timeout=2, verbose=False):

    with can.interface.Bus(channel=can_interface, bustype='socketcan') as bus:
        version_request_message = Message(message_types["Core_fw_version_request"], module_type, module_instance)
    
        try:
            bus.send(version_request_message.can_message())
            if verbose:
                print("Sending version request")
                print("Waiting for response...")
        except can.CanError as e:
            print(f"Failed to send message: {e}")
            exit(1)

        version = None
        start_time = time.time()
        while time.time() - start_time < timeout:
            response = bus.recv(timeout)
            if response:
                if response.is_extended_id:
                    message = Message(can_message=response)
                    if message.message_type == message_types["Core_fw_version_response"]:
                        version = FirmwareVersion.from_message(message)
                        break

        if not version:
            print("Failed to retrieve module version (timed out)")
            exit(1)
        
        return version

if __name__ == "__main__":
    # Parse command line arguments
    parser = argparse.ArgumentParser(description="Get version of module via CAN")
    parser.add_argument('--interface', type=str, default='can0', help='CAN interface to use, default can0')
    parser.add_argument('--timeout', type=int, default=1, help='Timeout in seconds, default 2')
    parser.add_argument('-t', '--type', type=str, help='Type of the module to check', required=True)
    parser.add_argument('-i', '--instance', type=str, help='Instance of the module to check', required=True)
    
    args = parser.parse_args()
    if  args.type not in module_types:
        print("unknown module type, valid types: \n",module_types.keys())
        exit(1)
    if  args.instance not in module_instances:
        print("unknown module instance, valid instances: \n",module_instances.keys())
        exit(1)

    version = check_version(args.interface, module_types[args.type], module_instances[args.instance], timeout=args.timeout, verbose=True)
    print("")
    print(f"Module version: {version}")



