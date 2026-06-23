import can
import argparse

from application_codes import Message, message_types, module_types, module_instances
from firmware_utils import FirmwareVersion


def check_version(can_interface, uid : str, timeout=2, verbose=False):

    uid_parsed = bytes.fromhex(uid)

    with can.interface.Bus(channel=can_interface, bustype='socketcan') as bus:
        version_request_message = Message(message_types["Core_fw_version_request"], module_types["Any"], module_instances["All"],data=uid_parsed)
    
        try:
            bus.send(version_request_message.can_message())
            if verbose:
                print("Sending version request")
                print("Waiting for response...")
        except can.CanError as e:
            print(f"Failed to send message: {e}")
            exit(1)
        response = Message(can_message=bus.recv(timeout))
        
    
        if verbose:
            print("Response recieved")
        version = FirmwareVersion.from_message(response)
        
        return version

if __name__ == "__main__":
    # Parse command line arguments
    parser = argparse.ArgumentParser(description="Get version of module via CAN")
    parser.add_argument('--interface', type=str, default='can0', help='CAN interface to use, default can0')
    parser.add_argument('--timeout', type=int, default=1, help='Timeout in seconds, default 2')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-u', '--uuid', type=str, help='UUID of the module to flash')
    
    args = parser.parse_args()

    version = check_version(args.interface, args.uuid, timeout=args.timeout, verbose=True)

    print("")
    print(f"Module version: {version}")



