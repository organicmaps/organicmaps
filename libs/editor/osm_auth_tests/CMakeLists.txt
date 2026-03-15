project(osm_auth_tests)

set(SRC
  osm_auth_tests.cpp
  server_api_test.cpp
)

# On Linux, OsmOAuth goes through the Qt-backed platform HttpClient implementation,
# so these tests need the Qt unit-test event loop/QCoreApplication bootstrap.
omim_add_test(${PROJECT_NAME} ${SRC} REQUIRE_QT)

target_link_libraries(${PROJECT_NAME} PRIVATE editor)
