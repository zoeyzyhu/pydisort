# Installs disort++ interface

set(namel disort++)
string(TOUPPER ${namel} nameu)

if (PYTHON_BINDINGS)
  set(compile_flags_ "${${PROJECT_NAME}_CXX_FLAGS} -fPIC")
else()
  set(compile_flags_ "${${PROJECT_NAME}_CXX_FLAGS}")
endif()

file(GLOB src_files 
    *.cpp
    )

string(TOLOWER ${CMAKE_BUILD_TYPE} buildl)
string(TOUPPER ${CMAKE_BUILD_TYPE} buildu)

add_library(${namel}_${buildl} 
  OBJECT
  ${src_files}
  )

set_target_properties(${namel}_${buildl}
  PROPERTIES 
  COMPILE_FLAGS ${compile_flags_}
  )

target_include_directories(${namel}_${buildl}
  PRIVATE 
  ${CDISORT_INCLUDE_DIR}
  )

target_link_libraries(${namel}_${buildl}
  pydisort::cdisort
  )

add_library(pydisort::disort++ ALIAS ${namel}_${buildl})
