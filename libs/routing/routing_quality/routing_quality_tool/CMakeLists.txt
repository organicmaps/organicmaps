project(routing_quality_tool)

set(SRC
  benchmark_results.cpp
  benchmark_results.hpp
  benchmark_stat.cpp
  benchmark_stat.hpp
  error_type_counter.cpp
  error_type_counter.hpp
  routing_quality_tool.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  routing_quality
  routing_api
  kml
  gflags::gflags
)
