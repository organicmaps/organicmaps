set(
  SRC
  config_loader_test.cpp
  editor_config_test.cpp
  editor_notes_test.cpp
  opening_hours_ui_test.cpp
  osm_feature_matcher_test.cpp
  ui2oh_test.cpp
  user_stats_test.cpp
  xml_feature_test.cpp
)

omim_add_test(editor_tests ${SRC})

omim_link_libraries(
  editor_tests
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
  tomcrypt
)

omim_link_libraries(
  editor_tests
  ${Qt5Widgets_LIBRARIES}
)

if (${PLATFORM_MAC})
  omim_link_libraries(
    editor_tests
    "-framework Cocoa"
    "-framework IOKit"
    "-framework SystemConfiguration"
  )
endif()
