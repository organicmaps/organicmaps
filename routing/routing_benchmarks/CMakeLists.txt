project(routing_benchmarks)

set(
  SRC
  ../routing_integration_tests/routing_test_tools.cpp
  ../routing_integration_tests/routing_test_tools.hpp
  bicycle_routing_tests.cpp
  helpers.cpp
  helpers.hpp
  pedestrian_routing_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  map
  routing
  traffic
  routing_common
  transit
  search
  storage
  mwm_diff
  traffic
  kml
  editor
  indexer
  platform
  oauthcpp
  opening_hours
  geometry
  coding
  base
  jansson
  protobuf
  bsdiff
  stats_client
  succinct
  pugixml
  icu
  agg
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
