file(GLOB IIR_FILTERS_SOURCES
    "iir/iir/*"
)

add_library(iir_filters ${IIR_FILTERS_SOURCES})

target_compile_options(iir_filters PUBLIC -Os)
set_target_properties(iir_filters PROPERTIES LINK_FLAGS -s)
target_compile_options(iir_filters PUBLIC -ffunction-sections)
set_target_properties(iir_filters PROPERTIES LINK_FLAGS -Wl,--gc-sections)

target_compile_definitions(iir_filters PUBLIC
    IIR1_NO_EXCEPTIONS
)

target_include_directories(iir_filters PUBLIC
    iir/iir/
)

