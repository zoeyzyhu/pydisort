string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

# Macro to create test for a given source file
macro(cdisort_setup_test name srcfile)
  add_executable(${name}.${buildl} ${srcfile} print_test.c)

  set_target_properties(
    ${name}.${buildl}
    PROPERTIES COMPILE_FLAGS ${CMAKE_C_FLAGS_${buildu}})

  target_include_directories(${name}.${buildl}
    PRIVATE
    ${DISORT_INCLUDE_DIR}/src
    ${DISORT_INCLUDE_DIR}/cdisort213)

  target_link_libraries(${name}.${buildl} pydisort::cdisort)

  add_test(NAME ${name}.${buildl} COMMAND ${name}.${buildl})
endmacro()

# Create two separate test executables, second one for large scale test
cdisort_setup_test(test_cdisort test_cdisort.c)
cdisort_setup_test(test_cdisort_09 test_cdisort_09.c)
