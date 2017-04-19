project(drape_tests)

execute_process(
  COMMAND python ${OMIM_ROOT}/tools/autobuild/shader_preprocessor.py
  ${DRAPE_ROOT}/shaders
  shader_index.txt
  shader_def
)

add_definitions(
  -DOGL_TEST_ENABLED
  -DGTEST_DONT_DEFINE_TEST
  -DCOMPILER_TESTS
)

include_directories(
  ${OMIM_ROOT}/3party/gmock/include
  ${OMIM_ROOT}/3party/gmock/gtest/include
  ${OMIM_ROOT}/3party/freetype/include
)

set(
  SRC
  attribute_provides_tests.cpp
  batcher_tests.cpp
  bidi_tests.cpp
  bingind_info_tests.cpp
  buffer_tests.cpp
  compile_shaders_test.cpp
  dummy_texture.hpp
  failure_reporter.cpp
  font_texture_tests.cpp
  glfunctions.cpp
  glmock_functions.cpp
  glmock_functions.hpp
  glyph_mng_tests.cpp
  glyph_packer_test.cpp
  img.cpp
  img.hpp
  memory_comparer.hpp
  pointers_tests.cpp
  static_texture_tests.cpp
  stipple_pen_tests.cpp
  texture_of_colors_tests.cpp
  testingmain.cpp
  uniform_value_tests.cpp
)

omim_add_executable(${PROJECT_NAME} ${DRAPE_COMMON_SRC} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  qt_tstfrm
  indexer
  platform
  coding
  geometry
  base
  icu
  expat
  stats_client
  freetype
  gmock
  stb_image
  sdf_image
  ${Qt5Core_LIBRARIES}
  ${Qt5Gui_LIBRARIES}
  ${Qt5Widgets_LIBRARIES}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    "-framework CoreLocation"
  )
endif()
