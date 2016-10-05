cmake_minimum_required(VERSION 3.2)

project(opening_hours C CXX)

include_directories(src ../../)

add_compile_options(
  "-Wall"
  "-std=c++11"
)


set(
  SRC
  opening_hours.hpp
  opening_hours.cpp
  opening_hours_parsers.hpp
  opening_hours_parsers_terminals.hpp
  parse_opening_hours.hpp
  parse_opening_hours.cpp
  rules_evaluation_private.hpp
  rules_evaluation.hpp
  rules_evaluation.cpp
)

add_library(opening_hours ${SRC})

omim_add_test_subdirectory(opening_hours_tests)
