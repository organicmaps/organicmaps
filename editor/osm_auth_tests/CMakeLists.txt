project(osm_auth_tests)

set(
  SRC
  osm_auth_tests.cpp
  server_api_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  editor
  indexer
  platform
  platform_tests_support
  geometry
  coding
  base
  pugixml
  protobuf
  jansson
  icu
  oauthcpp
  stats_client
  succinct
  ${LIBZ}
  ${Qt5Widgets_LIBRARIES}
)
