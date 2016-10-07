add_compile_options(
  "-Wall"
  "-std=c++11"
)

set(
  SRC
  test_results_matching.cpp
  test_results_matching.hpp
  test_search_engine.cpp
  test_search_engine.hpp
  test_search_request.cpp
  test_search_request.hpp
)

add_library(search_tests_support ${SRC})
