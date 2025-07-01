cmake_minimum_required(VERSION 3.16)

pico_sdk_init()

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND git describe --tags --abbrev=0 --dirty
    OUTPUT_VARIABLE GIT_LAST_TAG
    RESULT_VARIABLE RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Handle no tag existence
if(NOT RESULT EQUAL 0)
    set(GIT_LAST_TAG "v0.0")
endif()

# Extract version components
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" FW_VERSION_MAJOR "${GIT_LAST_TAG}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" FW_VERSION_MINOR "${GIT_LAST_TAG}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+-([0-9]+).*" "\\1" FW_VERSION_PATCH "${GIT_LAST_TAG}")
if("${FW_VERSION_PATCH}" STREQUAL "${GIT_LAST_TAG}")
    set(FW_VERSION_PATCH 0) # Default to 0 if no patch version is present v0.8 with no commit above
endif()

# Extract dirty flag for repo
execute_process(
    COMMAND git status --porcelain
    OUTPUT_VARIABLE GIT_STATUS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if("${GIT_STATUS}" STREQUAL "")
    set(FW_GIT_DIRTY false)
else()
    set(FW_GIT_DIRTY true)
endif()

# Print extracted variables to the terminal
message(STATUS "FW_VERSION_MAJOR: ${FW_VERSION_MAJOR}")
message(STATUS "FW_VERSION_MINOR: ${FW_VERSION_MINOR}")
message(STATUS "FW_VERSION_PATCH: ${FW_VERSION_PATCH}")
message(STATUS "GIT_COMMIT_HASH: ${GIT_COMMIT_HASH}")
message(STATUS "FW_GIT_DIRTY: ${FW_GIT_DIRTY}")

# Add definitions for compilation
add_definitions("-DFW_VERSION_MAJOR=${FW_VERSION_MAJOR}")
add_definitions("-DFW_VERSION_MINOR=${FW_VERSION_MINOR}")
add_definitions("-DFW_VERSION_PATCH=${FW_VERSION_PATCH}")
add_definitions("-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")
add_definitions("-DFW_GIT_DIRTY=${FW_GIT_DIRTY}")

set(COMPILER arm-none-eabi-)

set(CMAKE_C_COMPILER ${COMPILER}gcc)
set(CMAKE_CXX_COMPILER ${COMPILER}g++)
set(CMAKE_ASM_COMPILER ${COMPILER}gcc)
set(CMAKE_OBJCOPY ${COMPILER}objcopy)

set(TARGET firmware)

file(GLOB_RECURSE SOURCE_FILES
    "./*.cpp"
    "./*.c"
)

add_definitions("-DFW_COMPILER_NAME=\"${COMPILER}gcc\"")

string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--undefined=uxTopUsedPriority -Wl,--print-memory-usage ")

add_executable(${TARGET} ${SOURCE_FILES} )

target_compile_options(${TARGET} PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -Wall -Wextra -Wno-volatile>)
target_compile_definitions(${TARGET} PRIVATE PICO_CXX_DISABLE_ALLOCATION_OVERRIDES=1 PICO_NO_BINARY_INFO=1 PICO_STDIO_UART=0 PICO_XOSC_STARTUP_DELAY_MULTIPLIER=8)

target_include_directories(${TARGET} PUBLIC .)
target_include_directories(${TARGET} PUBLIC codes/messages)

pico_add_extra_outputs(${TARGET})

if(DEFINED CONFIG_BOOTLOADER)
    message(STATUS "Bootloader offset enabled: ${CONFIG_BOOTLOADER_OFFSET}")
    pico_set_linker_script(${TARGET} ${CMAKE_SOURCE_DIR}/bootloader/memmap_default.ld)
else()
    message(STATUS "Bootloader offset disabled")
endif()

target_link_libraries(${TARGET} can2040)
target_link_libraries(${TARGET} cmsis_core pico_stdlib pico_multicore pico_unique_id)
target_link_libraries(${TARGET} hardware_pwm hardware_pio hardware_i2c hardware_dma hardware_spi hardware_adc)
target_link_libraries(${TARGET} pico-lib)
target_link_libraries(${TARGET} tinyusb_device)
target_link_libraries(${TARGET} emio )
target_link_libraries(${TARGET} etl)
target_link_libraries(${TARGET} embedded-cli )
target_link_libraries(${TARGET} freertos)
target_link_libraries(${TARGET} freertos-addons)
target_link_libraries(${TARGET} iir_filters)
target_link_libraries(${TARGET} fast-hash)
target_link_libraries(${TARGET} lvgl::lvgl)
target_link_libraries(${TARGET} qlibs-cpp)

# Create output directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/out)

# Add custom command to copy firmware files
add_custom_target(copy-binary
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/source/firmware.elf
            ${CMAKE_SOURCE_DIR}/out/application.elf
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/source/firmware.uf2
            ${CMAKE_SOURCE_DIR}/out/application.uf2
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/source/firmware.bin
            ${CMAKE_SOURCE_DIR}/out/application.bin
    DEPENDS ${TARGET}
    COMMENT "Copying firmware files to output directory"
)

set(OUTPUT_NAME ${CONFIG_MODULE_TYPE})

# Create combined firmware target
add_custom_target(combined_firmware ALL
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/out
    COMMAND dd if=/dev/zero of=${CMAKE_SOURCE_DIR}/out/${OUTPUT_NAME}.bin bs=1k count=16 2> /dev/null
    COMMAND dd if=${CMAKE_SOURCE_DIR}/out/bootloader.bin of=${CMAKE_SOURCE_DIR}/out/${OUTPUT_NAME}.bin conv=notrunc 2> /dev/null
    COMMAND dd if=${CMAKE_SOURCE_DIR}/out/application.bin of=${CMAKE_SOURCE_DIR}/out/${OUTPUT_NAME}.bin bs=1k seek=16 conv=notrunc 2> /dev/null
    COMMAND picotool uf2 convert ${CMAKE_SOURCE_DIR}/out/${OUTPUT_NAME}.bin ${CMAKE_SOURCE_DIR}/out/${OUTPUT_NAME}.uf2
    COMMAND ln -sf ${OUTPUT_NAME}.bin firmware.bin
    COMMAND ln -sf ${OUTPUT_NAME}.uf2 firmware.uf2
    DEPENDS ${TARGET} katapult copy-binary
    COMMENT "Creating combined firmware binary (application & bootloader)"
)

# Add Katapult build target
add_custom_target(katapult
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/bootloader/katapult.config
        ${CMAKE_SOURCE_DIR}/bootloader/katapult/.config
    COMMAND make -C ${CMAKE_SOURCE_DIR}/bootloader/katapult
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/bootloader/katapult/out/katapult.bin
        ${CMAKE_SOURCE_DIR}/out/bootloader.bin
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/bootloader/katapult/out/katapult.elf
        ${CMAKE_SOURCE_DIR}/out/bootloader.elf
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/bootloader/katapult/out/katapult.uf2
        ${CMAKE_SOURCE_DIR}/out/bootloader.uf2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building Katapult bootloader"
)
