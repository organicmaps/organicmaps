project(poly_borders)

set(
  SRC
  borders_data.cpp
  borders_data.hpp
  help_structures.cpp
  help_structures.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} generator)

omim_add_test_subdirectory(poly_borders_tests)

omim_add_tool_subdirectory(poly_borders_tool)
