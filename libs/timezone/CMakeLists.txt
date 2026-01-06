project(timezone)

set(SRC
  serdes.cpp
  serdes.hpp
  timezone.hpp
  timezone.cpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
add_library(om::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
omim_add_test_subdirectory(timezone_tests)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  base
  coding
  glaze::glaze
  platform
)
