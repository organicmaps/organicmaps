project(geo_index_benchmark_tool)

set(SRC geo_index_benchmark_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    indexer
    platform
    gflags::gflags
)
