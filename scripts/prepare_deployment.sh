#!/bin/bash

# Function to generate firmware
generate_firmware() {
    local module_name=$1

    # Set configuration
    setconfig --kconfig=config/Kconfig "${module_name}=y"

    # Generate config header
    genconfig  --header-path source/config.hpp config/Kconfig

    # Build firmware
    make

    # Convert module name to lowercase
    local module_name_lowercase="${module_name,,}"

    # Copy firmware to binaries folder with new name
    cp build/source/firmware.bin "binaries/${module_name_lowercase}.bin"
}

build_bootloader(){
    # Copy bootloader configuration into submodule
    cp bootloader/katapult.config bootloader/katapult/.config

    # Build bootloader
    make -C bootloader/katapult/

    # Copy bootloader to binaries folder
    cp bootloader/katapult/out/katapult.uf2 binaries/katapult.uf2

}

# Prepare folder for binaries
rm -rf binaries
mkdir -p binaries

# Path to config file
export KCONFIG_CONFIG=config/.config

# Array of module names
modules=("CONTROL_MODULE" "SENSOR_MODULE")

# Generate default configuration
alldefconfig config/Kconfig

# Loop through each module and call the generate_firmware function
for module in "${modules[@]}"; do
    echo "Generating firmware for ${module}..."
    generate_firmware "$module"
    echo "Firmware generation completed for ${module}."
done

# Build bootloader
build_bootloader

echo "All firmware builds completed."

# Pack files into deplay archive
archive_name="smbr_firmware.zip"

git rev-parse HEAD > commit.txt

rm -f ${archive_name}
zip -r ${archive_name} binaries
zip -j ${archive_name} scripts/*.py scripts/Readme.md commit.txt

rm -f commit.txt
