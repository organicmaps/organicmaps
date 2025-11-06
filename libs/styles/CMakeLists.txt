project(styles)

set(SRC
  map_style_config.hpp
  map_style_manager.cpp
  map_style_manager.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  coding
  PRIVATE
  platform
  base
  glaze::glaze
)

omim_add_test_subdirectory(styles_tests)
