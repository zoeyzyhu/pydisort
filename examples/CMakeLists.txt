
file(GLOB inputs *.inp *.dat *.yaml)
foreach(input ${inputs})
  execute_process(COMMAND ln -sf ${input} ${CMAKE_BINARY_DIR}/bin/${inp})
endforeach()

# Register the Python examples as tests. Each one is self-checking: it ends
# with assertions against an analytic solution or a conservation law, so a
# non-zero exit status means the example no longer reproduces its own results.
file(GLOB example_scripts example_*.py)

foreach(script ${example_scripts})
  get_filename_component(name ${script} NAME)
  configure_file(${script} ${CMAKE_CURRENT_BINARY_DIR}/${name} COPYONLY)
  add_test(NAME examples.${name} COMMAND ${Python3_EXECUTABLE} ${name}
           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()
