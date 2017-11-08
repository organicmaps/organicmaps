project(search_integration_tests)

set(
  SRC
  downloader_search_test.cpp
  generate_tests.cpp
  helpers.cpp
  helpers.hpp
  interactive_search_test.cpp
  pre_ranker_test.cpp
  processor_test.cpp
  ranker_test.cpp
  search_edited_features_test.cpp
  smoke_test.cpp
  tracer_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator_tests_support
  search_tests_support
  indexer_tests_support
  generator
  routing
  routing_common
  search
  storage
  stats_client
  indexer
  editor
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
  ${Qt5Network_LIBRARIES}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
