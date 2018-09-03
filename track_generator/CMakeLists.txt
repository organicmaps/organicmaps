project(track_generator_tool)

include_directories(
  ${OMIM_ROOT}/3party/gflags/src
)

set(
  SRC
  track_generator_tool.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  routing_quality
  routing_common
  routing
  kml
  indexer
  platform
  bsdiff
  geometry
  coding
  base
  icu
  protobuf
  expat
  stats_client
  gflags
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
