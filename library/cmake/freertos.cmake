
if(false)
    file(GLOB FREERTOS_SOURCES
        "freertos/*.c"
        "freertos/portable/GCC/ARM_CM0/port.c"
        "freertos/portable/MemMang/heap_3.c"
    )

    add_library(freertos ${FREERTOS_SOURCES})

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 20)

    target_include_directories(freertos PUBLIC
        freertos/include
        freertos/portable/GCC/ARM_CM0
        ../source/config
    )

    target_compile_options(freertos PUBLIC -Wno-shadow -Wno-use-after-free)
endif()

if(true)
    include(freertos/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

    add_library(freertos)

    target_link_libraries(freertos FreeRTOS-Kernel FreeRTOS-Kernel-Heap3)

    target_include_directories(freertos PUBLIC ../source/config)

    target_compile_options(freertos PUBLIC -Wno-shadow -Wno-use-after-free)
endif()


