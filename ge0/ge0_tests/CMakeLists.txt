project(ge0_tests)

set(SRC
  geo_url_tests.cpp
  parser_tests.cpp
  url_generator_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} ge0)
