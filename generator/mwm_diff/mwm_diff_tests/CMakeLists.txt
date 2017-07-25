project(mwm_diff_tests)

set(
  SRC
  diff_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator_tests_support
  platform_tests_support
  mwm_diff
  generator
  drape_frontend
  map
  routing
  search
  storage
  indexer
  drape
  traffic
  routing_common
  editor
  platform
  geometry
  coding
  base
  freetype
  expat
  icu
  jansson
  protobuf
  osrm
  stats_client
  minizip
  succinct
  pugixml
  tess2
  gflags
  oauthcpp
  opening_hours
  stb_image
  sdf_image
  # TODO(syershov): Use FindPackage.
  sqlite3
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
