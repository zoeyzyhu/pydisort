# Install disrot library
#
#   library: libdisort_debug.o
#   library: libdisort_release.o
#
# Define the following symbols
#
#   DISORT_INCLUDE_DIR
#   pydisort::disort
#
# Normal usage would be:
#
#   include_directories(${DISORT_INCLUDE_DIR})
#   target_link_libraries(pydisort::disort)


include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  SYSTEM ${TORCH_API_INCLUDE_DIR}
  SYSTEM ${TORCH_INCLUDE_DIR}
  )

string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

set(namel disort)
string(TOUPPER ${namel} nameu)

file(GLOB src_files *.cpp)

add_library(${namel}_${buildl}
    OBJECT
    ${src_files}
    )

set_target_properties(${namel}_${buildl}
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS_${buildu}}"
    )

target_include_directories(${namel}_${buildl}
    PRIVATE "${FMT_INCLUDE_DIR}"
    PRIVATE "${CDISORT_INCLUDE_DIR}"
    )

target_link_libraries(${namel}_${buildl} pydisort::cdisort)

add_library(pydisort::disort ALIAS ${namel}_${buildl})

set(DISORT_INCLUDE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}"
  CACHE INTERNAL "disort include directory")
