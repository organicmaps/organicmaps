project(samples_generation_tool)

set(SRC samples_generation_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  search_quality
  search_tests_support
  gflags::gflags
)
