project(storage_tests)

add_definitions("-DOMIM_UNIT_TEST_WITH_QT_EVENT_LOOP")

set(
  SRC
  country_info_getter_test.cpp
  country_name_getter_test.cpp
  fake_map_files_downloader.cpp
  fake_map_files_downloader.hpp
  helpers.cpp
  helpers.hpp
  queued_country_tests.cpp
  simple_tree_test.cpp
  storage_tests.cpp
  task_runner.cpp
  task_runner.hpp
  test_map_files_downloader.cpp
  test_map_files_downloader.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  drape_frontend
  map
  traffic
  storage
  drape
  generator_tests_support
  generator
  search
  routing
  routing_common
  ugc
  indexer
  platform_tests_support
  editor
  oauthcpp
  kml
  platform
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
  tess2
  protobuf
  stats_client
  minizip
  succinct
  pugixml
  stb_image
  sdf_image
  ${Qt5Widgets_LIBRARIES}
  ${Qt5Network_LIBRARIES}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
