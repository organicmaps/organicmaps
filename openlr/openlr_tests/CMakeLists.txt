project(openlr_tests)

set(
  SRC
  decoded_path_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  platform_tests_support
  generator_tests_support
  platform_tests_support
  generator
  routing
  search
  storage
  openlr
  editor
  indexer
  routing_common
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
  traffic
  ${CMAKE_DL_LIBS}
  ${Qt5Core_LIBRARIES}
  ${LIBZ}
)
