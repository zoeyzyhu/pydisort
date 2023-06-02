string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

# 1. Compile test_cppdisort.c

add_executable(test_cppdisort.${buildl} test_cppdisort.cpp)

set_target_properties(
  test_cppdisort.${buildl}
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
             COMPILE_FLAGS ${CMAKE_CXX_FLAGS_${buildu}})

target_link_libraries(test_cppdisort.${buildl} ${PYDISORT_LIBRARY_${buildu}}
                      ${PYTHON_LIBRARY_RELEASE})

target_include_directories(
  test_cppdisort.${buildl}
  PRIVATE ${PYDISORT_INCLUDE_DIR} ${CMAKE_BINARY_DIR} ${TOMLPP_INCLUDE_DIR}
          ${PYTHON_INCLUDE_DIR} ${PYBIND11_INCLUDE_DIR})

# 1. Copy input file to run directory

file(GLOB inputs *.inp *.toml)
foreach(input ${inputs})
  get_filename_component(name ${input} NAME)
  configure_file(${input} ${CMAKE_BINARY_DIR}/bin/${name} COPYONLY)
endforeach()
