project(api)

set(
  SRC
  ge0_generator.cpp
  ge0_generator.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(api_tests)
