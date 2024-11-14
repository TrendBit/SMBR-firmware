from flashtool import *

import os
import sys
from contextlib import contextmanager

@contextmanager
def suppress_stdout():
    with open(os.devnull, 'w') as devnull:
        old_stdout = sys.stdout
        sys.stdout = devnull
        try:
            yield
        finally:
            sys.stdout = old_stdout

async def query_can_nodes(can_interface):
    loop = asyncio.get_running_loop()
    sock = CanSocket(loop)
    try:
        sock.cansock.bind((can_interface,))
    except Exception:
        raise FlashError("Unable to bind socket to can0")
    sock.closed = False
    sock.cansock.setblocking(False)
    sock._loop.add_reader(
        sock.cansock.fileno(), sock._handle_can_response)

    with suppress_stdout():
        sock._reset_nodes()
        await asyncio.sleep(.5)
        nodes = await sock._query_uuids()

    nodes_id = [format(node, 'x') for node in nodes]

    return nodes_id

def query_katapult_nodes(args):
    return asyncio.run(query_can_nodes(args))

if __name__ == "__main__":
    logging.getLogger().setLevel(logging.ERROR)
    # Parse command line arguments
    parser = argparse.ArgumentParser(description="Query available katapult nodes")
    parser.add_argument('--interface', type=str, default='can0', help='CAN interface to use, default can0')
    args = parser.parse_args()

    print("Resetting all bootloader node IDs")
    print("Waiting for responses...")

    katapult_nodes = query_katapult_nodes(args.interface)

    if katapult_nodes:
        print("Found the following Katapult nodes:")
        for node in katapult_nodes:
            print(node)
    else:
        print("No Katapult nodes found")




