# Install cdisort submodule
#
#   library: libcdisort_debug.o
#   library: libcdisort_release.o
#
# Define the following symbols
#
#   CDISORT_INCLUDE_DIR
#   pydisort::cdisort
#
# Normal usage would be:
#
#   include_directories(${CDISORT_INCLUDE_DIR})
#   target_link_libraries(pydisort::cdisort)

string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

set(namel cdisort)
string(TOUPPER ${namel} nameu)

file(GLOB src_files *.c)

add_library(${namel}_${buildl}
    STATIC
    ${src_files}
    )

set_target_properties(${namel}_${buildl}
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_C_FLAGS_${buildu}}"
    )

target_link_libraries(${namel}_${buildl} m)

add_library(pydisort::cdisort ALIAS ${namel}_${buildl})

set(CDISORT_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "CDISORT include directory")
