file(GLOB_RECURSE PICO_LIB_SOURCES
    "pico-lib/*.cpp"
)

add_library(pico-lib ${PICO_LIB_SOURCES})

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

message(STATUS "CONFIG_DIR: ${CONFIG_DIR}")

target_include_directories(pico-lib PUBLIC
    ${CONFIG_DIR}/..
    pico-lib/
)

file(GLOB_RECURSE PICO_LIB_PIO_FILES
    "pico-lib/pio/*.pio"
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../pico-lib/pio_generated)

foreach(FILE ${PICO_LIB_PIO_FILES})
    pico_generate_pio_header(pico-lib ${FILE} OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/../pico-lib/pio_generated)
endforeach()

target_compile_options(pico-lib PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -Wall -Wextra -Wno-volatile>)
target_compile_definitions(pico-lib PRIVATE PICO_CXX_DISABLE_ALLOCATION_OVERRIDES=1 PICO_STDIO_UART=0)

target_link_libraries(pico-lib pico_stdlib )
target_link_libraries(pico-lib hardware_pwm hardware_pio hardware_i2c hardware_dma hardware_spi hardware_adc)
target_link_libraries(pico-lib tinyusb_device)
target_link_libraries(pico-lib emio )
target_link_libraries(pico-lib embedded-cli )
target_link_libraries(pico-lib freertos)
target_link_libraries(pico-lib freertos-addons)
