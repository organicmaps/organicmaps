project(timezone_tests)

set(SRC
  conversion_tests.cpp
  local_timezone_tests.cpp
  serdes_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC} GTEST)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  timezone
)
