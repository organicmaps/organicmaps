project(world_feed_integration_tests)

set(SRC world_feed_integration_tests.cpp)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  world_feed
  generator
)
