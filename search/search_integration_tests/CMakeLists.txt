project(search_integration_tests)

set(
  SRC
  downloader_search_test.cpp
  generate_tests.cpp
  postcode_points_tests.cpp
  pre_ranker_test.cpp
  processor_test.cpp
  ranker_test.cpp
  search_edited_features_test.cpp
  smoke_test.cpp
  tracer_tests.cpp
  utils_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  search_tests_support
  editor_tests_support
  generator_tests_support
  platform_tests_support
  generator
  routing
  routing_common
  search
  storage
  stats_client
  editor
  indexer
  ge0
  platform
  mwm_diff
  bsdiff
  geometry
  coding
  base
  oauthcpp
  tess2
  protobuf
  jansson
  succinct
  pugixml
  opening_hours
  icu
  traffic
  ${CMAKE_DL_LIBS}
  ${Qt5Network_LIBRARIES}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
