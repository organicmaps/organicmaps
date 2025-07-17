project(openlr_stat)

set(SRC openlr_stat.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  openlr
  storage
  platform
  gflags::gflags
)
