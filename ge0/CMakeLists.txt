project(ge0)

set(
  SRC
  parser.cpp
  parser.hpp
  url_generator.cpp
  url_generator.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(ge0_tests)
