import time 

import can
import argparse

from application_codes import Message, message_types, module_types, module_instances

def print_to_oled(can_interface, message : str, verbose=False):
    with can.interface.Bus(channel=can_interface, bustype='socketcan') as bus:
        # split the message into chunks of 8 characters (to fit into the message)
        message_chunks : list[str] = []
        while message:
            message_chunks.append(message[:8])
            message = message[8:]
        
        for chunk in message_chunks:
            can_message = Message(message_types["Mini_OLED_print_custom_text"], module_types["Any"], module_instances["All"],data=chunk.encode())
    
            try:
                bus.send(can_message.can_message())
                
                # wait for 50ms so the buffer can catch up
                time.sleep(0.05)
                if verbose:
                    print(f"Sending oled print chunk: '{chunk}'")
            except can.CanError as e:
                raise Exception(f"Failed to send message: {e}")

def clear_oled(can_interface, verbose = False):
    with can.interface.Bus(channel=can_interface, bustype='socketcan') as bus:
        can_message = Message(message_types["Mini_OLED_clear_custom_text"], module_types["Any"], module_instances["All"])

        try:
            bus.send(can_message.can_message())
            if verbose:
                print(f"Sending clear oled message")
        except can.CanError as e:
            raise Exception(f"Failed to send message: {e}")
    
def replace_oled_text(can_interface, message : str, verbose = False):
    clear_oled(can_interface, verbose)
    time.sleep(0.1)
    print_to_oled(can_interface, message, verbose)
    
if __name__ == "__main__":
    # Parse command line arguments
    parser = argparse.ArgumentParser(description="Append text to oled or clear the display")
    parser.add_argument('--interface', type=str, default='can0', help='CAN interface to use, default can0')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-t', '--text', type=str, help='What to print')
    group.add_argument('-c', '--clear', action='store_true', help='Clear display')
    group.add_argument('-r', '--replace', type=str, help='Clear display')
    
    args = parser.parse_args()
    if args.replace:
        replace_oled_text(args.interface, args.replace, verbose=True)
    if args.text:
        print_to_oled(args.interface, args.text, verbose=True)
    if args.clear:
        clear_oled(args.interface, verbose=True)
