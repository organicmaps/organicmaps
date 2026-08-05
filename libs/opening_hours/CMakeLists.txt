project(opening_hours)

set(SRC
  opening_hours.hpp
  opening_hours.cpp
  oh/parser.hpp
  oh/eval.hpp
  oh/convert.hpp
  oh/convert.cpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

# PUBLIC: opening_hours.hpp includes timezone/timezone.hpp in its interface.
target_link_libraries(${PROJECT_NAME} PUBLIC om::timezone)

# osmoh facade + issue-regression tests, plus the port's golden-vector suites
# (transcribed from the opening-hours-rs Rust tests). All on the OM framework.
omim_add_test_subdirectory(opening_hours_tests)
