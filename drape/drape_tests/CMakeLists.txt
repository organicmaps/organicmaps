project(drape_tests)

add_definitions(
  -DOGL_TEST_ENABLED
  -DGTEST_DONT_DEFINE_TEST
  -DWITH_GL_MOCK
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
  dummy_texture.hpp
  failure_reporter.cpp
  font_texture_tests.cpp
  gl_functions.cpp
  gl_mock_functions.cpp
  gl_mock_functions.hpp
  glyph_mng_tests.cpp
  glyph_packer_test.cpp
  img.cpp
  img.hpp
  memory_comparer.hpp
  object_pool_tests.cpp
  pointers_tests.cpp
  static_texture_tests.cpp
  stipple_pen_tests.cpp
  texture_of_colors_tests.cpp
  testing_graphics_context.hpp
  uniform_value_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${DRAPE_COMMON_SRC} ${VULKAN_SRC} ${SRC})

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
  oauthcpp
  freetype
  gmock
  stb_image
  sdf_image
  vulkan_wrapper
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

if (PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    dl
  )
endif()
