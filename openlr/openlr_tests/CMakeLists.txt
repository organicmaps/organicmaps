project(openlr_tests)

set(
  SRC
  decoded_path_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator_tests_support
  platform_tests_support
  generator
  routing
  search
  openlr
  indexer
  routing_common
  editor
  platform_tests_support
  platform
  coding
  geometry
  base
  jansson
  tess2
  oauthcpp
  opening_hours
  pugixml
  stats_client
  succinct
  protobuf
  icu
  ${Qt5Core_LIBRARIES}
  ${LIBZ}
)
