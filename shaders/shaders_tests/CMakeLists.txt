project(shaders_tests)

execute_process(
  COMMAND rm -rf ${CMAKE_BINARY_DIR}/shaders_compiler
)

execute_process(
  COMMAND cp -r ${OMIM_ROOT}/tools/shaders_compiler ${CMAKE_BINARY_DIR}/shaders_compiler
)

set(
  SRC
  gl_shaders_desktop_compile_tests.cpp
  gl_program_params_tests.cpp
  # Mobile compilation test takes much more time than others, so it should be the last.
  gl_shaders_mobile_compile_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
  )
endif()

omim_link_libraries(
  ${PROJECT_NAME}
  qt_tstfrm
  shaders
  drape
  platform
  indexer
  geometry
  coding
  base
  expat
  stats_client
  oauthcpp
  freetype
  stb_image
  sdf_image
  icu
  vulkan_wrapper
  ${LIBZ}
)

if (PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    dl
  )
endif()

link_opengl(${PROJECT_NAME})
link_qt5_core(${PROJECT_NAME})
