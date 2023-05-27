# =============================
# Create the cdisort library
# =============================

set(namel cdisort)
string(TOUPPER ${namel} nameu)

file(GLOB src_files cdisort.c locate.c)

string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

add_library(${namel}_${buildl} STATIC ${src_files})

set_target_properties(${namel}_${buildl} PROPERTIES COMPILE_FLAGS
                                                    ${CMAKE_C_FLAGS_${buildu}})

target_include_directories(${namel}_${buildl} PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(${namel}_${buildl} m)

set(${nameu}_LIBRARY_${buildu}
    ${namel}_${buildl}
    CACHE STRING "${namel} library ${build}")
mark_as_advanced(${nameu}_LIBRARY_${build})
