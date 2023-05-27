string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

# Compile test_cdisort.c

add_executable(test_cdisort.${buildl} test_cdisort.c print_test.c)

set_target_properties(
  test_cdisort.${buildl}
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
             COMPILE_FLAGS ${CMAKE_C_FLAGS_${buildu}})

target_link_libraries(test_cdisort.${buildl} ${CDISORT_LIBRARY_${buildu}})

target_include_directories(test_cdisort.${buildl} PRIVATE ${CDISORT_INCLUDE_DIR}
                                                          ${CMAKE_BINARY_DIR})
