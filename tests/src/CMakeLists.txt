string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)

macro(cppdisort_setup_test namel)
  add_executable(${namel}.${buildl} ${namel}.cpp)

  set_target_properties(${namel}.${buildl}
                        PROPERTIES COMPILE_FLAGS ${${PROJECT_NAME}_CXX_FLAGS})

  target_include_directories(${namel}.${buildl} PRIVATE ${PYDISORT_INCLUDE_DIR})

  target_link_libraries(${namel}.${buildl} pydisort::disort++)

  add_test(NAME ${namel}.${buildl} COMMAND ${namel}.${buildl})
endmacro()

cppdisort_setup_test(test_cppdisort)
