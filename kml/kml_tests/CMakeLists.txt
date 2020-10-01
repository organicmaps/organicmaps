project(kml_tests)

set(
  SRC
  serdes_tests.cpp
  tests_data.hpp
  minzoom_quadtree_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  kml
  indexer
  editor
  platform
  coding
  geometry
  base
  icu
  jansson
  oauthcpp
  protobuf
  pugixml
  expat
  stats_client
  succinct
  ${LIBZ}
)
link_qt5_core(${PROJECT_NAME})
