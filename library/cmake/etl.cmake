add_library(etl INTERFACE)

target_compile_features(etl INTERFACE cxx_std_20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

target_include_directories(etl INTERFACE
    etl/include/
)


