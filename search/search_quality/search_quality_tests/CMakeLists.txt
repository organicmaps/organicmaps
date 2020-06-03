project(search_quality_tests)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(SRC sample_test.cpp)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  search_quality
  search
  indexer
  editor
  ge0
  platform
  coding
  geometry
  base
  icu
  jansson
  protobuf
  oauthcpp
  opening_hours
  pugixml
  stats_client
  succinct
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
