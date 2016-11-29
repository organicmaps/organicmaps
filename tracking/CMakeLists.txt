project(tracking)

set(
  SRC
  connection.cpp
  connection.hpp
  protocol.cpp
  protocol.hpp
  reporter.cpp
  reporter.hpp
)

add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(tracking_tests)
add_subdirectory(pytracking)
