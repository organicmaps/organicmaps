cmake_minimum_required(VERSION 3.2)

project(opening_hours C CXX)

add_definitions(-DBOOST_SPIRIT_USE_PHOENIX_V3)

include_directories(src ../../)

add_compile_options(
  "-Wall"
  "-std=c++11"
)

add_clang_compile_options("-Wno-unused-local-typedef")
add_gcc_compile_options(
  "-Wno-unused-local-typedef"
  "-Wno-return-type"
)

set(
  SRC
  opening_hours.hpp
  opening_hours.cpp
  opening_hours_parsers.hpp
  opening_hours_parsers_terminals.cpp
  parse_opening_hours.hpp
  parse_opening_hours.cpp
  parse_years.cpp
  parse_weekdays.cpp
  parse_weeks.cpp
  parse_timespans.cpp
  parse_months.cpp
  rules_evaluation_private.hpp
  rules_evaluation.hpp
  rules_evaluation.cpp
)

add_library(opening_hours ${SRC})

omim_add_test_subdirectory(opening_hours_tests)
omim_add_test_subdirectory(opening_hours_integration_tests)
omim_add_test_subdirectory(opening_hours_supported_features_tests)
