project(routing_common_tests)

set(
  SRC
  vehicle_model_for_country_test.cpp
  vehicle_model_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  indexer
  platform_tests_support
  editor
  traffic
  routing_common
  platform
  oauthcpp
  geometry
  coding
  base
  osrm
  protobuf
  succinct
  opening_hours
  jansson
  stats_client
  map
  pugixml
  stats_client
  icu
  agg
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
