project(osm_auth_tests)

set(SRC
  osm_auth_tests.cpp
  server_api_test.cpp
)

# On Linux, OsmOAuth goes through the Qt-backed platform HttpClient implementation,
# so these tests need the Qt unit-test event loop/QCoreApplication bootstrap.
omim_add_test(${PROJECT_NAME} ${SRC} REQUIRE_QT)

target_link_libraries(${PROJECT_NAME} PRIVATE editor)

# OSM_Auth_Login authenticates against the OSM dev server over HTTPS. On Ubuntu 24.04
# (Qt 6.4.2) the HTTP/2 reply can fail to emit finished() until the peer closes the
# connection, so the blocking HttpClient::RunHttpRequest() hangs far past its timeout
# and the test stalls until the global CTest timeout. Cap the run so CI fails fast.
# Qt bug (fixed in 6.5.1 / 6.6.0): https://bugreports.qt.io/browse/QTBUG-111417
# Remove this once the CI Qt is newer than 6.4.
set_tests_properties(${PROJECT_NAME} PROPERTIES TIMEOUT 60)
