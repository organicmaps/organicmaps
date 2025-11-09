project(srtm_coverage_checker)

set(SRC
  srtm_coverage_checker.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    generator
    routing_common
    platform
    indexer
    gflags::gflags
)
