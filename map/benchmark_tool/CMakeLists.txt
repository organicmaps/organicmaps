project(benchmark_tool)

set(SRC
  api.cpp
  api.hpp
  features_loading.cpp
  main.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  map
  gflags::gflags
)
