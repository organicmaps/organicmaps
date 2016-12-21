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
  search_edited_features_test.cpp
  smoke_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator_tests_support
  search_tests_support
  indexer_tests_support
  generator
  routing
  search
  storage
  stats_client
  indexer
  editor
  platform
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
  ${Qt5Core_LIBRARIES}
  ${Qt5Network_LIBRARIES}
  ${LIBZ}
)
