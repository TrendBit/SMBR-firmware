file(GLOB FAST_HASH_SOURCES
    "fast-hash/*.c"
)

add_library(fast-hash ${FAST_HASH_SOURCES})

target_include_directories(fast-hash PUBLIC
    fast-hash/
)

