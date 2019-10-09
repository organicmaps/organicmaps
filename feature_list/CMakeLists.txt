project(feature_list_generator)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(SRC feature_list.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  routing_common
  search_quality
  search_tests_support
  search
  storage
  ugc
  kml
  editor
  indexer
  platform
  mwm_diff
  bsdiff
  geometry
  generator
  coding
  base
  agg
  icu
  jansson
  oauthcpp
  opening_hours
  protobuf
  pugixml
  stats_client
  succinct
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
