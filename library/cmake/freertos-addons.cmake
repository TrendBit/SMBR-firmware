file(GLOB FRA_SOURCES
    "freertos-addons/c/Source/*"
    "freertos-addons/c++/Source/*"
)

add_library(freertos-addons ${FRA_SOURCES})

target_compile_options(freertos-addons PUBLIC -Wno-shadow -Wno-use-after-free)

target_compile_definitions(freertos-addons PUBLIC
    CPP_FREERTOS_NO_EXCEPTIONS
    DO_NOT_INCLUDE_TRACE=1
)

target_include_directories(freertos-addons PUBLIC
    freertos/FreeRTOS/Source/include
    freertos/FreeRTOS/Source/CMSIS_RTOS_V2
    freertos/FreeRTOS/Source/portable/GCC/ARM_CM0
    freertos-addons/c/Source/include
    freertos-addons/c++/Source/include
    ../source/config
)

target_link_libraries(freertos-addons freertos)
