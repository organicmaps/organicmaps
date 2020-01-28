project(ge0_tests)

set(
  SRC
  parser_tests.cpp
  url_generator_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  ge0
  platform
  coding
  base
  oauthcpp
  stats_client
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
