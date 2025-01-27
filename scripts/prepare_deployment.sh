#!/bin/bash

# Function to generate firmware
generate_firmware() {
    local module_name=$1

    # Set configuration
    setconfig --kconfig=config/Kconfig "${module_name}=y"

    # Generate config header
    genconfig  --header-path source/config.hpp config/Kconfig

    # Build firmware
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1
    make firmware -j$(nproc)
    cd ..

    # Convert module name to lowercase
    local module_name_lowercase="${module_name,,}"

    # Copy firmware to binaries folder with new name
    cp build/source/firmware.bin "binaries/${module_name_lowercase}.bin"
}

build_bootloader(){
    # Copy bootloader configuration into submodule
    cp bootloader/katapult.config bootloader/katapult/.config

    # Build bootloader
    cd bootloader/katapult
    make
    cd ../..

    # Copy bootloader to binaries folder
    cp bootloader/katapult/out/katapult.uf2 binaries/katapult.uf2

}

# Prepare folder for binaries
rm -rf binaries
mkdir -p binaries


# Path to config file
export KCONFIG_CONFIG=config/.config

# Backup current config
cp ${KCONFIG_CONFIG} config/.config.backup

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

# Restore original config
mv config/.config.backup ${KCONFIG_CONFIG}

# Build bootloader
build_bootloader

echo "All firmware builds completed."

# Get version branch and commit
version=$(git describe --tags | sed 's/\(.*\)-.*/\1/')
if [ $? -ne 0 ]; then
  version="vx.x"
fi

branch=$(git rev-parse --abbrev-ref HEAD)
commit=$(git describe --always --dirty)

# Generate the version.txt file
cat > version.txt <<EOF
version: ${version}
branch: ${branch}
commit: ${commit}
EOF

# Pack files into deplay archive
archive_name="smpbr_firmware_${version}.zip"

rm -f ${archive_name}
zip -r ${archive_name} binaries
zip -j ${archive_name} scripts/*.py scripts/Readme.md version.txt

rm -f version.txt
