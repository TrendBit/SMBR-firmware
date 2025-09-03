#!/bin/bash
# Automatic flashing script for RaspberryPi MCU (RP2040)
# Run: ./auto_single_flash.sh <binary.uf2>
# Switches device to BOOTSEL, flashes firmware, repeats automatically.

# --- Configuration ---
SERIAL_PORT="/dev/ttyACM0"
MOUNT_POINT="/media/$USER/RPI-RP2"

# --- Check arguments ---
check_arguments() {
    if [ "$#" -ne 1 ]; then
        echo "Usage: $0 <binary.uf2>"
        exit 1
    fi
    if [ ! -f "$1" ]; then
        echo "Binary $1 not found!"
        exit 1
    fi
}

# --- Send bootloader command over USB serial ---
send_bootloader_command() {
    if [ -e "$SERIAL_PORT" ]; then
        echo "Sending 'bootloader' command to $SERIAL_PORT..."
        sudo stty -F "$SERIAL_PORT" 115200 cs8 -cstopb -parenb -ixon -ixoff -crtscts raw -echo
        echo -e "bootloader\r" | sudo tee "$SERIAL_PORT" > /dev/null
        sleep 2
    else
        echo "Serial port $SERIAL_PORT not found. Waiting for manual BOOTSEL..."
    fi
}

# --- Wait until RP2040 is mounted ---
wait_for_mount() {
    echo "Waiting for RP2040 mount at $MOUNT_POINT ..."
    while [ ! -d "$MOUNT_POINT" ]; do
        sleep 0.5
    done
    echo "Mount point found: $MOUNT_POINT"
}

# --- Copy/flash firmware ---
flash_device() {
    local binary=$1
    echo "Flashing firmware to $MOUNT_POINT ..."
    cp -v "$binary" "$MOUNT_POINT"/ || return 1
}

# --- Wait for device to disconnect ---
wait_for_flash() {
    echo -n "Waiting for device to finish flashing..."
    while [ -d "$MOUNT_POINT" ]; do
        sleep 0.5
    done
    echo "Done."
}

# --- Main loop ---
main() {
    local binary=$1

    check_arguments "$@"
    echo "Starting automatic flash mode. Press Ctrl+C to exit."

    echo "Requesting BOOTSEL mode and waiting for RP2040 device..."
    send_bootloader_command
    wait_for_mount
    flash_device "$binary" || exit 1
    wait_for_flash

    echo -e "\nFlash successful!"
    exit 0
}

main "$@"
