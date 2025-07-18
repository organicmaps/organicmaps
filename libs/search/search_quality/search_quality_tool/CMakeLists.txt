project(search_quality_tool)

set(SRC search_quality_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  search_tests_support
  search_quality
  gflags::gflags
)
