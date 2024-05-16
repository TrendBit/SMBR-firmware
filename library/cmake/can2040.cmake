file(GLOB CAN2040_SOURCES
    "can2040/src/can2040.c"
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_library(can2040 ${CAN2040_SOURCES})

target_compile_options(can2040 PUBLIC -Wno-shadow -Wno-use-after-free)

target_include_directories(can2040 PUBLIC
    can2040/src
)

file(GLOB_RECURSE CAN2040_PIO_FILES
    "can2040/pio/*.pio"
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../can2040/pio_generated)

foreach(FILE ${PICO_LIB_PIO_FILES})
    pico_generate_pio_header(can2040 ${FILE} OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/../can2040/pio_generated)
endforeach()

target_compile_options(can2040 PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -Wall -Wextra -Wno-volatile>)
target_compile_definitions(can2040 PRIVATE PICO_CXX_DISABLE_ALLOCATION_OVERRIDES=1 PICO_STDIO_UART=0)

target_link_libraries(can2040 pico_stdlib )
target_link_libraries(can2040 hardware_pio hardware_dma)
