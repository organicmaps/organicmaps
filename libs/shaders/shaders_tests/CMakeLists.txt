project(shaders_tests)

set(SRC
  gl_shaders_desktop_compile_tests.cpp
  gl_program_params_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

# It takes much more time than the other tests, so it should be the last.
if ((PLATFORM_LINUX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64") OR PLATFORM_MAC)
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
