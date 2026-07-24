project(drape_frontend_tests)

set(SRC
  area_pattern_gpu_test.cpp
  area_shape_tests.cpp
  bicycle_line_types_tests.cpp
  clip_splines_builder_tests.cpp
  frame_values_tests.cpp
  navigator_test.cpp
  path_text_test.cpp
  rainbow_line_gpu_test.cpp
  screen_operations_tests.cpp
  shape_test_fixture.cpp
  shape_test_fixture.hpp
  stylist_tests.cpp
  user_event_stream_tests.cpp
  visual_params_fixture.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE drape_frontend qt_tstfrm)
