project(drape_frontend_tests)

add_definitions(
  -DCOMPILER_TESTS
)

execute_process(
  COMMAND python ${OMIM_ROOT}/tools/autobuild/shader_preprocessor.py
  ${OMIM_ROOT}/drape_frontend/shaders
  shader_index.txt
  shaders_lib.glsl
  ${OMIM_ROOT}/drape_frontend/drape_frontend_tests
  shader_def_for_tests
)

execute_process(
  COMMAND rm -rf ${OMIM_ROOT}/data/shaders_compiler
)

execute_process(
  COMMAND cp -r ${OMIM_ROOT}/tools/shaders_compiler ${OMIM_ROOT}/data
)

set(
  SRC
  compile_shaders_test.cpp
  navigator_test.cpp
  path_text_test.cpp
  shader_def_for_tests.cpp
  shader_def_for_tests.hpp
  user_event_stream_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
  )
endif()

omim_link_libraries(
  ${PROJECT_NAME}
  drape_frontend
  drape
  platform
  indexer
  geometry
  coding
  base
  expat
  stats_client
  freetype
  stb_image
  sdf_image
  icu
  ${LIBZ}
)

link_opengl(${PROJECT_NAME})
link_qt5_core(${PROJECT_NAME})
