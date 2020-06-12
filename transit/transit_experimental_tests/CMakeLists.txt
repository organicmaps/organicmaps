project(transit_experimental_tests)

set(
  SRC
  parse_transit_from_json_tests.cpp
  transit_serdes_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  indexer
  transit
  platform
  coding
  geometry
  base
  jansson
  stats_client
  opening_hours
  oauthcpp
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
