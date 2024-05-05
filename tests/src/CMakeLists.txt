string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)

# Compile test_cdisort.c

macro(cdisort_setup_test namel)
  add_executable(${namel}.${buildl} test_cdisort.c print_test.c)

  set_target_properties(${namel}.${buildl}
                        PROPERTIES COMPILE_FLAGS ${${PROJECT_NAME}_C_FLAGS})

  target_include_directories(${namel}.${buildl}
                             PRIVATE ${PYDISORT_INCLUDE_DIR}/src)

  target_link_libraries(${namel}.${buildl} pydisort::cdisort)

  add_test(NAME ${namel}.${buildl} COMMAND ${namel}.${buildl})
endmacro()

cdisort_setup_test(test_cdisort)
