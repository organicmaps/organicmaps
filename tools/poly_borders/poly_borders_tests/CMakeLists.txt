project(poly_borders_tests)

set(SRC
  mark_points_tests.cpp
  remove_empty_spaces_tests.cpp
  tools.cpp
  tools.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  poly_borders
  platform_tests_support
)
