# =============================
# Create the cppdisort library
# =============================

set(namel cppdisort)
string(TOUPPER ${namel} nameu)

file(GLOB src_files cppdisort.cc)

string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

add_library(${namel}_${buildl}
    STATIC
    ${src_files})

set_target_properties(${namel}_${buildl}
    PROPERTIES
    COMPILE_FLAGS ${CMAKE_C_FLAGS_${buildu}}
    )

target_include_directories(${namel}_${buildl}
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${TOMLPP_INCLUDE_DIR}
    ${CDISORT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIR}
    ${PYBIND11_INCLUDE_DIR}
    )

target_link_libraries(${namel}_${buildl}
    ${CDISORT_LIBRARY_${buildu}}
    ${PYTHON_LIBRARIES}
    )

set(${nameu}_LIBRARY_${buildu} ${namel}_${buildl}
    CACHE STRING
    "${namel} library ${build}")
mark_as_advanced(${nameu}_LIBRARY_${build})
