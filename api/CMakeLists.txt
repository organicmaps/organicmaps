project(api)

set(
  SRC
  ge0_generator.cpp
  ge0_generator.hpp
  ge0_parser.cpp
  ge0_parser.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(api_tests)
