file(GLOB EMBCLI_SOURCES "embedded-cli/lib/src/*")

add_library(embedded-cli ${EMBCLI_SOURCES})

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

target_include_directories(embedded-cli PUBLIC
    embedded-cli/lib/include
)
