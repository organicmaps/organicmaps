project(generator_tests_support)

set(
  SRC
  routing_helpers.cpp
  routing_helpers.hpp
  test_feature.cpp
  test_feature.hpp
  test_mwm_builder.cpp
  test_mwm_builder.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
