project(shaders_tests)

execute_process(
  COMMAND rm -rf ${CMAKE_BINARY_DIR}/shaders_compiler
)

execute_process(
  COMMAND cp -r ${OMIM_ROOT}/tools/shaders_compiler ${CMAKE_BINARY_DIR}/shaders_compiler
)

set(SRC
  gl_shaders_desktop_compile_tests.cpp
  gl_program_params_tests.cpp
  # Mobile compilation test takes much more time than others, so it should be the last.
  gl_shaders_mobile_compile_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  qt_tstfrm
  shaders
  platform
)
