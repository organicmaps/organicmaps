project(track_generator_tool)

set(SRC
  track_generator_tool.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  routes_builder
  routing
  kml
  coding
  expat::expat
  gflags::gflags
)
