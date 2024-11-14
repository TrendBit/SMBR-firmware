import can
import time
import argparse

from application_codes import *


def identify_modules(can_interface, timeout=2, verbose=False):

    bus = can.interface.Bus(channel=can_interface, bustype='socketcan')

    probe_request_message = Message(message_types["Probe_modules_request"], module_types["Any"], module_instances["All"])

    try:
        bus.send(probe_request_message.can_message())
        if verbose:
            print("Sending module probe request")
            print("Waiting for responses...")
    except can.CanError as e:
        if verbose:
            print(f"Failed to send message: {e}")
        exit(0)

    start_time = time.time()

    probe_responses = []
    while time.time() - start_time < timeout:
        response = bus.recv(timeout)
        if response:
            if response.is_extended_id:
                probe_responses.append(Message(can_message=response))

    modules = []
    for resp in probe_responses:
        if resp.message_type == message_types["Probe_modules_response"]:
            modules.append(Module(resp))

    return modules

if __name__ == "__main__":
    # Parse command line arguments
    parser = argparse.ArgumentParser(description="CAN Bus Send and Listen")
    parser.add_argument('--interface', type=str, default='can0', help='CAN interface to use, default can0')
    parser.add_argument('--timeout', type=int, default=1, help='Timeout in seconds, default 2')
    args = parser.parse_args()

    modules = identify_modules(args.interface, timeout=args.timeout, verbose=True)

    if modules:
        print(f"\nIdentified modules: {len(modules)}\n")
        print(f"{'Module type':<25}{'Instance':<20}{'UID'}")
        print(f"{'-'*25}{'-'*20}{'-'*12}")
        for module in modules:
            print(f"{module.module_name():<25}{module.instance_name():<20}{module.uid_str()}")
    else:
        print("No identification response received.")



