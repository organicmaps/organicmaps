project(storage_integration_tests)

include_directories(${OMIM_ROOT}/3party/glm)
add_definitions("-DOMIM_UNIT_TEST_WITH_QT_EVENT_LOOP")

set(
  SRC
  migrate_tests.cpp
  storage_3levels_tests.cpp
  storage_downloading_tests.cpp
  storage_group_download_tests.cpp
  storage_http_tests.cpp
  storage_update_tests.cpp
  test_defines.cpp
  test_defines.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  map
  drape_frontend
  routing
  routing_common
  transit
  search
  storage
  tracking
  traffic
  ugc
  indexer
  drape
  partners_api
  local_ads
  platform_tests_support
  platform
  editor
  opening_hours
  mwm_diff
  bsdiff
  geometry
  coding
  base
  freetype
  expat
  icu
  agg
  jansson
  protobuf
  osrm
  stats_client
  minizip
  succinct
  pugixml
  oauthcpp
  stb_image
  sdf_image
  ${Qt5Widgets_LIBRARIES}
  ${Qt5Network_LIBRARIES}
  ${LIBZ}
)

link_opengl(${PROJECT_NAME})
link_qt5_core(${PROJECT_NAME})
