project(mapshot_tool)

set(SRC mapshot.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  map
  software_renderer
  gflags::gflags
)
