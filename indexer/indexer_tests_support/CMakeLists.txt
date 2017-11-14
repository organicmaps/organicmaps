project(indexer_tests_support)

set(
  SRC
  helpers.cpp
  helpers.hpp
  test_with_classificator.cpp
  test_with_classificator.hpp
  test_with_custom_mwms.cpp
  test_with_custom_mwms.hpp
)

add_library(${PROJECT_NAME} ${SRC})
