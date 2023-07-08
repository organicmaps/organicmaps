project(drape_frontend_tests)

set(SRC
  frame_values_tests.cpp
  navigator_test.cpp
  path_text_test.cpp
  stylist_tests.cpp
  user_event_stream_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} drape_frontend)
