project(tracking_tests)

set(
  SRC
  archival_reporter_tests.cpp
  protocol_test.cpp
  reporter_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  tracking
  routing
  platform
  platform_tests_support
  traffic
  coding
  base
  geometry
  oauthcpp
  minizip
  ${LIBZ}
  ${Qt5Widgets_LIBRARIES}
)
