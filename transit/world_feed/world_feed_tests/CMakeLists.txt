project(world_feed_tests)

set(SRC
  subway_converter_tests.cpp
  world_feed_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  generator
  world_feed
  transit
)
