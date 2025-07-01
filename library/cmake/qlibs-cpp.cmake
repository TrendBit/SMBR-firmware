file(GLOB_RECURSE QLIB_CPP_SOURCES
    "qlibs-cpp/src/*.cpp"
)

add_library(qlibs-cpp ${QLIB_CPP_SOURCES})

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

target_include_directories(qlibs-cpp PUBLIC
    qlibs-cpp/src
)