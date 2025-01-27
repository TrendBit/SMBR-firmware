#!/bin/bash
# Automatic flashing script for RaspberryPi MCU (RP2040)
# Run: ./auto_flash.sh <binary.uf2>
# Detects newly connected device in BOOT mode (built in USb bootloader)
#   mount then and flash the provided UF2 file. Repeats until stooped.
# Inspired by pico-blackMagic-flashTool by usePAT

# Check arguments and if UF2 exists
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

# Detect device with RP2 descriptor
find_device() {
    RP2040_DEVICE=$(lsblk -o NAME,MODEL -nr | grep "RP2" | cut -d' ' -f1)
    if [ -z "$RP2040_DEVICE" ]; then
        return 1
    fi
    echo "${RP2040_DEVICE}"
}

# Mount first partition of device
mount_device() {
    local device=$1
    udisksctl mount -b /dev/${device}1 2>/dev/null || return 1
}

# Copy/flash firmware to device
flash_device() {
    local binary=$1
    echo -n "Flashing firmware: "
    cp -v "$binary" /media/$USER/RPI-RP2/ ||return 1
}

# Wait for device to disconnect
wait_for_flash() {
    local device=$1
    local count=0
    echo -n "Waiting fo device to finish..."
    while lsblk -o NAME,MODEL -nr | grep -q "RP2"; do
        sleep 0.2
        count=$((count+1))
        if [ $count -ge 50 ]; then
            echo "Flash timeout"
            udisksctl unmount -b /dev/${device}1
            return 1
        fi
    done
    return 0
}

main() {
    local binary=$1
    local flash_count=0

    check_arguments "$@"
    echo "Starting continuous flash mode. Press Ctrl+C to exit."

    while true; do
        echo -en "\rWaiting for RP2040 device (${flash_count} flashed)..."

        # Wait for device
        while true; do
            device=$(find_device) && break
            sleep 0.5
        done

        echo -e "\nDevice found: ${device}, mounting in progress..."

        # Wait for mount
        while ! mount_device "$device"; do
            sleep 0.5
        done

        flash_device "$binary" || continue
        wait_for_flash "$device" || continue

        flash_count=$((flash_count + 1))
        echo -e "\nFlash successful! Total flashed: ${flash_count}"
    done
}

main "$@"
