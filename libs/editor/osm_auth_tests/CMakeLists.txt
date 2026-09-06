project(osm_auth_tests)

set(SRC
  osm_auth_tests.cpp
  server_api_test.cpp
)

# On Linux, OsmOAuth goes through the Qt-backed platform HttpClient implementation,
# so these tests need the Qt unit-test event loop/QCoreApplication bootstrap.
omim_add_test(${PROJECT_NAME} ${SRC} REQUIRE_QT)
# Extends the "omim-test" label set by omim_add_test.
set_property(TEST ${PROJECT_NAME} APPEND PROPERTY LABELS "omim-live-test")

target_link_libraries(${PROJECT_NAME} PRIVATE editor)
