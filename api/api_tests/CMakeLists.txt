project(api_tests)

set(
  SRC
  ge0_generator_tests.cpp
  ge0_parser_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  api
  platform
  coding
  base
  oauthcpp
  stats_client
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
