project(shaders_tests)

set(SRC
  gl_shaders_desktop_compile_tests.cpp
  gl_program_params_tests.cpp
  # Mobile compilation test takes much more time than others, so it should be the last.
  gl_shaders_mobile_compile_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

# External GLSL compilers used by gl_shaders_mobile_compile_test. They are only executed, never
# written to, so the test runs them right from the source tree, no matter which directory it is
# started from.
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    OMIM_SHADERS_COMPILERS_DIR="${OMIM_ROOT}/tools/shaders_compiler"
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    qt_tstfrm
    shaders
    platform
)
