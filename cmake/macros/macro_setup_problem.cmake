# A small macro used for setting up the build of a problem.
#
# Usage: setup_problem(name)

string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

macro(setup_problem namel)
  add_executable(${namel}.${buildl} ${namel}.cpp)

  set_target_properties(
    ${namel}.${buildl}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
               COMPILE_FLAGS ${CMAKE_CXX_FLAGS_${buildu}})

  target_include_directories(
    ${namel}.${buildl}
    PRIVATE ${CMAKE_BINARY_DIR}
            ${DISORT_INCLUDE_DIR}
            SYSTEM
            ${TORCH_INCLUDE_DIR}
            SYSTEM
            ${TORCH_API_INCLUDE_DIR})

  target_link_libraries(${namel}.${buildl}
    PRIVATE pydisort::disort
            pydisort::disort_cu
            ${TORCH_LIBRARIES})
endmacro()
