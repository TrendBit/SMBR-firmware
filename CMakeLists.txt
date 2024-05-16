cmake_minimum_required(VERSION 3.16)

include(pico_sdk_import.cmake)

# Set the build type (Debug or Release)
set(CMAKE_BUILD_TYPE Debug)

# Set the Pico SDK build type
set(PICO_DEFAULT_BUILD_TYPE Debug)

project(PhenoCAN CXX C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

set(MCU_FLAGS "-mcpu=cortex-m0plus")
set(COMMON_FLAGS "-Wall -Wextra -Wshadow -Wformat=2 -Wformat-truncation")
set(CMAKE_C_FLAGS "${COMMON_FLAGS} ${MCU_FLAGS}")
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} ${MCU_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics -Wno-volatile")

set(CONFIG_DIR ${CMAKE_SOURCE_DIR}/source/config)

add_subdirectory(library)

add_subdirectory(source)
