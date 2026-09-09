project(shaders_tests)

set(SRC
  gl_shaders_desktop_compile_tests.cpp
  gl_program_params_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

# The mobile compilation test runs the compilers bundled in tools/shaders_compiler, which exist for
# Linux and macOS only. It takes much more time than the other tests, so it should be the last.
if (PLATFORM_LINUX OR PLATFORM_MAC)
  target_sources(${PROJECT_NAME}
    PRIVATE
      gl_shaders_mobile_compile_test.cpp
  )

  # The compilers are only executed, never written to, so run them from the source tree regardless
  # of the test's working directory.
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      OMIM_SHADERS_COMPILERS_DIR="${OMIM_ROOT}/tools/shaders_compiler"
  )
endif()

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    qt_tstfrm
    shaders
    platform
)
