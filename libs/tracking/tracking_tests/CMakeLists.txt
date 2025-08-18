project(tracking_tests)

set(
  SRC
  archival_reporter_tests.cpp
  protocol_test.cpp
  reporter_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(${PROJECT_NAME}
  platform_tests_support
  tracking
  routing
)
