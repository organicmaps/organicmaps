project(drape_tests)

set(SRC
  attribute_provides_tests.cpp
  batcher_tests.cpp
  bidi_tests.cpp
  bingind_info_tests.cpp
  buffer_tests.cpp
  dummy_texture.hpp
  font_texture_tests.cpp
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
  vertex_buffer_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_compile_definitions(${PROJECT_NAME}
  PRIVATE GTEST_DONT_DEFINE_TEST WITH_GL_MOCK
)

find_package_or_fallback_to_3party(
  "GTest"
  "1.12.0"
  "${CMAKE_CURRENT_LIST_DIR}/../../3party/googletest"
  "${CMAKE_CURRENT_BINARY_DIR}/googletest"
)

target_link_libraries(${PROJECT_NAME}
  qt_tstfrm
  indexer  # For StyleReader in static_texture_tests
  drape
  gmock
  Qt5::Core
)
