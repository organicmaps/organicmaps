project(generator_tests_support)

set(
  SRC
  routing_helpers.cpp
  routing_helpers.hpp
  test_feature.cpp
  test_feature.hpp
  test_mwm_builder.cpp
  test_mwm_builder.hpp
  test_with_classificator.cpp
  test_with_classificator.hpp
  test_with_custom_mwms.cpp
  test_with_custom_mwms.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
