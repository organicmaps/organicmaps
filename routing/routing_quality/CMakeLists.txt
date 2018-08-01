project(routing_quality)

set(
  SRC
  utils.cpp
  utils.hpp
  waypoints.cpp
  waypoints.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator
  routing
  traffic
  routing_common
  transit
  storage
  indexer
  platform
  mwm_diff
  bsdiff
  geometry
  coding
  base
  icu
  jansson
  protobuf
  stats_client
  ${LIBZ}
)

omim_add_test_subdirectory(routing_quality_tests)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
