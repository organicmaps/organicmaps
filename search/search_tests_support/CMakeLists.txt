project(search_tests_support)

set(
  SRC
  test_results_matching.cpp
  test_results_matching.hpp
  test_search_engine.cpp
  test_search_engine.hpp
  test_search_request.cpp
  test_search_request.hpp
  test_with_classificator.cpp
  test_with_classificator.hpp
  test_with_custom_mwms.cpp
  test_with_custom_mwms.hpp
)

add_library(${PROJECT_NAME} ${SRC})
