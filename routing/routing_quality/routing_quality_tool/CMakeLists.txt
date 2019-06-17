project(routing_quality_tool)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  routing_quality_tool.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  routing_quality
  routing_api
  routes_builder
  generator
  routing
  platform
  traffic
  routing_common
  transit
  storage
  indexer
  platform
  mwm_diff
  bsdiff
  kml
  geometry
  coding
  base
  icu
  jansson
  oauthcpp
  protobuf
  stats_client
  gflags
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
