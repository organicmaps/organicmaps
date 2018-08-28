project(metrics_tests)

set(
  SRC
  eye_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})
omim_link_libraries(
  ${PROJECT_NAME}
  metrics_tests_support
  metrics
  platform
  coding
  base
  stats_client
  jansson
  ${LIBZ})

link_qt5_core(${PROJECT_NAME})
