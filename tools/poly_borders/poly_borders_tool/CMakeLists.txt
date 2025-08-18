project(poly_borders_tool)

set(SRC poly_borders_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  poly_borders
  gflags::gflags
)
