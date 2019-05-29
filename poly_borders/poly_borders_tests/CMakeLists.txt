project(poly_borders_tests)

set(
  SRC
  mark_points_tests.cpp
  remove_empty_spaces_tests.cpp
  tools.cpp
  tools.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  poly_borders
  platform_tests_support
  generator
  routing
  traffic
  routing_common
  descriptions
  transit
  ugc
  search
  storage
  editor
  indexer
  mwm_diff
  platform
  geometry
  coding
  base
  pugixml
  opening_hours
  icu
  jansson
  protobuf
  stats_client
  succinct
  tess2
  gflags
  oauthcpp
  sqlite3
  ${CMAKE_DL_LIBS}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
