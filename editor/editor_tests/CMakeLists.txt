project(editor_tests)

set(
  SRC
  config_loader_test.cpp
  editor_config_test.cpp
  editor_notes_test.cpp
  feature_matcher_test.cpp
  match_by_geometry_test.cpp
  opening_hours_ui_test.cpp
  osm_editor_test.cpp
  osm_editor_test.hpp
  ui2oh_test.cpp
  user_stats_test.cpp
  xml_feature_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  editor
  platform_tests_support
  generator_tests_support
  generator
  indexer_tests_support
  indexer
  routing
  routing_common
  search
  succinct
  storage
  tess2
  jansson
  icu
  protobuf
  platform
  geometry
  coding
  base
  stats_client
  opening_hours
  pugixml
  oauthcpp
  ${LIBZ}
  ${Qt5Widgets_LIBRARIES}
)
