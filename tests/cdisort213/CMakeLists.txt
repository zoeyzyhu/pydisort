string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)

# Compile test_cdisort.c

string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

macro(cdisort_setup_test namel)
  add_executable(${namel}.${buildl} test_cdisort.c print_test.c)

  set_target_properties(
    ${namel}.${buildl}
    PROPERTIES COMPILE_FLAGS ${CMAKE_C_FLAGS_${buildu}})

  target_include_directories(${namel}.${buildl}
    PRIVATE
    ${DISORT_INCLUDE_DIR}/src
    ${DISORT_INCLUDE_DIR}/cdisort213)

  target_link_libraries(${namel}.${buildl} pydisort::cdisort)

  add_test(NAME ${namel}.${buildl} COMMAND ${namel}.${buildl})
endmacro()

cdisort_setup_test(test_cdisort)
