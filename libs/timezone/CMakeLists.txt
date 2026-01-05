project(timezone)

set(SRC
  local_timezone.cpp
  local_timezone.hpp
  serdes.cpp
  serdes.hpp
  timezone.hpp
  timezone.cpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
omim_add_test_subdirectory(timezone_tests)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  base
  coding
  glaze::glaze
  platform
)
