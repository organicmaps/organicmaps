project(storage_tests)

add_definitions("-DOMIM_UNIT_TEST_WITH_QT_EVENT_LOOP")

set(
  SRC
  country_info_getter_tests.cpp
  country_name_getter_tests.cpp
  fake_map_files_downloader.cpp
  fake_map_files_downloader.hpp
  helpers.cpp
  helpers.hpp
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
  platform_tests_support
  generator_tests_support
  drape_frontend
  shaders
  map
  ge0
  storage
  drape
  generator
  search
  routing
  traffic
  routing_common
  ugc
  kml
  editor
  indexer
  oauthcpp
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
  vulkan_wrapper
  ${Qt5Widgets_LIBRARIES}
  ${Qt5Network_LIBRARIES}
  ${LIBZ}
)

if (PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    dl
  )
endif()

link_qt5_core(${PROJECT_NAME})
