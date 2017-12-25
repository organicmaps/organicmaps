project(routing_benchmarks)

set(
  SRC
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
  search
  storage
  indexer
  platform
  editor
  oauthcpp
  opening_hours
  geometry
  coding
  base
  jansson
  protobuf
  stats_client
  succinct
  pugixml
  icu
  agg
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
