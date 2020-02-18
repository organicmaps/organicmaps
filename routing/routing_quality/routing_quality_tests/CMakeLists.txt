project(routing_quality_tests)

set(
  SRC
  barriers_tests.cpp
  bigger_roads_tests.cpp
  ferry_tests.cpp
  is_built_tests.cpp
  leaps_postprocessing_tests.cpp
  passby_roads_tests.cpp
  waypoints_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  routes_builder
  routing_quality
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
  opening_hours
  jansson
  oauthcpp
  protobuf
  stats_client
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
