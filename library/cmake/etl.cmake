Include(FetchContent)

FetchContent_Declare(
  etl
  GIT_REPOSITORY https://github.com/ETLCPP/etl
  GIT_TAG        20.38.15
)

FetchContent_MakeAvailable(etl)

if(false)
    add_library(etl INTERFACE)

    target_compile_features(etl INTERFACE cxx_std_20)

    set(GIT_DIR_LOOKUP_POLICY ALLOW_LOOKING_ABOVE_CMAKE_SOURCE_DIR)

    target_include_directories(etl INTERFACE
        etl/include/etl
        etl/include/etl/atomic
        etl/include/etl/deprecated
        etl/include/etl/experimental
        etl/include/etl/generators
        etl/include/etl/mutex
        etl/include/etl/private
        etl/include/etl/profiles
    )
endif()



