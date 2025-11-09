project(generator_tool)

set(SRC generator_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    generator
    gflags::gflags
)
