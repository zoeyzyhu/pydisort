# compile cdisort

set(namel cdisort)
string(TOUPPER ${namel} nameu)

if(PYTHON_BINDINGS)
  set(compile_flags_ "${${PROJECT_NAME}_C_FLAGS} -fPIC")
else()
  set(compile_flags_ "${${PROJECT_NAME}_C_FLAGS}")
endif()

file(GLOB src_files *.c)

string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

add_library(${namel}_${buildl} STATIC ${src_files})

set_target_properties(${namel}_${buildl} PROPERTIES COMPILE_FLAGS
                                                    ${compile_flags_})

target_include_directories(${namel}_${buildl} PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(${namel}_${buildl} m)

add_library(pydisort::cdisort ALIAS ${namel}_${buildl})

set(CDISORT_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "CDISORT include directory")
