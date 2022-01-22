project(ge0)

set(SRC
  geo_url_parser.cpp
  geo_url_parser.hpp
  parser.cpp
  parser.hpp
  url_generator.cpp
  url_generator.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(ge0_tests)
