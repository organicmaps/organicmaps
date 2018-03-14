project(srtm_coverage_checker)

include_directories(
  ${OMIM_ROOT}/3party/gflags/src
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  srtm_coverage_checker.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator
  map
  routing
  routing_common
  traffic
  indexer
  editor
  kml
  platform
  geometry
  coding
  base
  agg
  gflags
  icu
  jansson
  minizip
  oauthcpp
  opening_hours
  protobuf
  pugixml
  stats_client
  succinct
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
