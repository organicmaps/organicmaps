project(eye_tests)

set(
  SRC
  eye_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})
omim_link_libraries(
  ${PROJECT_NAME}
  eye_tests_support
  eye
  platform
  coding
  base
  stats_client
  jansson
  ${LIBZ})

link_qt5_core(${PROJECT_NAME})
