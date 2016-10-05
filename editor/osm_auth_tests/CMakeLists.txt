set(
  SRC
  osm_auth_tests.cpp
  server_api_test.cpp
)

omim_add_test(osm_auth_tests ${SRC})

omim_link_libraries(
  osm_auth_tests
  editor
  platform
  platform_tests_support
  geometry
  coding
  base
  pugixml
  oauthcpp
  tomcrypt
)

target_link_libraries(
  osm_auth_tests
  ${Qt5Widgets_LIBRARIES}
)

if (${PLATFORM_MAC})
  target_link_libraries(
    osm_auth_tests
    ${Qt5Widgets_LIBRARIES}
    "-framework Cocoa"
    "-framework IOKit"
    "-framework SystemConfiguration"
  )
endif()
