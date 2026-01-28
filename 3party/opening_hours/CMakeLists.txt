project(opening_hours)

set(SRC
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

omim_add_library(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} INTERFACE .)

omim_add_test_subdirectory(opening_hours_tests)
omim_add_test_subdirectory(opening_hours_integration_tests)
omim_add_test_subdirectory(opening_hours_supported_features_tests)
