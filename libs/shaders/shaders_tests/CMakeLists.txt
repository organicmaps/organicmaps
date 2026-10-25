project(shaders_tests)

set(SRC)
if (BUILD_OPENGL_TESTS)
  list(APPEND SRC
    gl_shaders_desktop_compile_tests.cpp
    gl_program_params_tests.cpp
  )
endif()

# It takes much more time than the other tests, so it should be the last.
if ((PLATFORM_LINUX AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64") OR PLATFORM_MAC)
  list(APPEND SRC gl_shaders_mobile_compile_test.cpp)
endif()

if (NOT SRC)
  return()
endif()

omim_add_test(${PROJECT_NAME} ${SRC})

# The compilers are only executed, never written to, so run them from the source tree regardless
# of the test's working directory.
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
