project(map_integration_tests)

set(
  SRC
  interactive_search_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  map
  search_tests_support
  editor_tests_support
  generator_tests_support
  generator
  routing
  routing_common
  search
  storage
  stats_client
  editor
  indexer
  metrics
  platform
  mwm_diff
  bsdiff
  geometry
  coding
  base
  oauthcpp
  tess2
  protobuf
  jansson
  succinct
  pugixml
  opening_hours
  icu
  ${Qt5Network_LIBRARIES}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
