project(track_generator_tool)

set(
  SRC
  track_generator_tool.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} gflags::gflags)

omim_link_libraries(
  ${PROJECT_NAME}
  routing_quality
  routes_builder
  routing
  routing_common
  kml
  storage
  indexer
  platform
  mwm_diff
  bsdiff
  geometry
  coding
  base
  icu
  expat
  oauthcpp
  protobuf
  opening_hours
  succinct
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
