project(editor_tests)

set(
  SRC
  config_loader_test.cpp
  editor_config_test.cpp
  editor_notes_test.cpp
  feature_matcher_test.cpp
  match_by_geometry_test.cpp
  opening_hours_ui_test.cpp
  ui2oh_test.cpp
  user_stats_test.cpp
  xml_feature_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  editor
  platform_tests_support
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
