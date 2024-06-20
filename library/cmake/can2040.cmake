file(GLOB CAN2040_SOURCES
    "can2040/src/*.c"
)

message(STATUS "CAN2040_SOURCES: ${CAN2040_SOURCES}")

add_library(can2040 ${CAN2040_SOURCES})

target_include_directories(can2040 PUBLIC
    can2040/src
)

target_compile_options(can2040 PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -Wall -Wextra -Wno-volatile>)
target_compile_definitions(can2040 PRIVATE PICO_CXX_DISABLE_ALLOCATION_OVERRIDES=1 PICO_STDIO_UART=0)

target_link_libraries(can2040 cmsis_core pico_stdlib hardware_pio hardware_dma)
