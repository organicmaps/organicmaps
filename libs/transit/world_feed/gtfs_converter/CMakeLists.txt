project(gtfs_converter)

omim_add_executable(${PROJECT_NAME} gtfs_converter.cpp)

target_link_libraries(${PROJECT_NAME}
  world_feed
  generator
  platform
  gflags::gflags
)
