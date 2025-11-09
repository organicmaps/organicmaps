project(features_collector_tool)

set(SRC features_collector_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    search_quality
    search_tests_support
    gflags::gflags
)
