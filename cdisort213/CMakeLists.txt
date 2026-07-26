# Install cdisort submodule
#
# cdisort is header-only since the CUDA port: cdisort.c/locate.c were
# split into DISPATCH_MACRO inline headers (see cdisort.hpp), so the
# target is a pure INTERFACE library.
#
# Define the following symbols
#
#   pydisort::cdisort
#
# Normal usage would be:
#
#   target_link_libraries(pydisort::cdisort)

string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)

set(namel cdisort)

add_library(${namel}_${buildl} INTERFACE)

target_include_directories(${namel}_${buildl}
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

target_link_libraries(${namel}_${buildl} INTERFACE m)

add_library(pydisort::cdisort ALIAS ${namel}_${buildl})
