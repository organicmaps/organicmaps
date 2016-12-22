project(map_tests)

add_definitions("-DOMIM_UNIT_TEST_WITH_QT_EVENT_LOOP")

set(
  SRC
  address_tests.cpp
  bookmarks_test.cpp
  chart_generator_tests.cpp
  feature_getters_tests.cpp
  ge0_parser_tests.cpp
  geourl_test.cpp
  gps_track_collection_test.cpp
  gps_track_storage_test.cpp
  gps_track_test.cpp
  kmz_unarchive_test.cpp
  mwm_url_tests.cpp
)

if (NOT PLATFORM_LINUX)
  append(SRC working_time_tests.cpp)
endif()

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  map
  drape_frontend
  routing
  traffic
  search
  storage
  tracking
  drape
  indexer
  partners_api
  platform
  editor
  geometry
  coding
  base
  freetype
  fribidi
  expat
  protobuf
  jansson
  osrm
  stats_client
  minizip
  succinct
  pugixml
  stats_client
  opening_hours
  oauthcpp
  drape_frontend
  drape
  ${Qt5Core_LIBRARIES}
  ${LIBZ}
)

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
    "-framework OpenGL"
  )
endif()


if (PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Network_LIBRARIES}
    GL
  )
endif()