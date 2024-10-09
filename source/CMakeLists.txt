cmake_minimum_required(VERSION 3.16)

pico_sdk_init()

execute_process(
    COMMAND git describe --abbrev=7 --dirty --always
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

if(NOT RESULT EQUAL 0)
    set(GIT_LAST_TAG "None")
endif()

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

string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--undefined=uxTopUsedPriority -Wl,--print-memory-usage ")

add_definitions("-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")
add_definitions("-DFW_VERSION=\"${GIT_LAST_TAG}\"")
add_definitions("-DCOMPILER_NAME=\"${COMPILER}gcc\"")

add_executable(${TARGET} ${SOURCE_FILES} )

target_compile_options(${TARGET} PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -Wall -Wextra -Wno-volatile>)
target_compile_definitions(${TARGET} PRIVATE PICO_CXX_DISABLE_ALLOCATION_OVERRIDES=1 PICO_STDIO_UART=0)

target_include_directories(${TARGET} PUBLIC .)


pico_add_extra_outputs(${TARGET})

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
