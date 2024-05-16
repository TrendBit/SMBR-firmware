add_library(emio INTERFACE)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

target_include_directories(emio INTERFACE
    emio/include/
)


