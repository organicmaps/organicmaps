project(feature_segments_checker)

include_directories(
  ${OMIM_ROOT}/3party/gflags/src
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  feature_segments_checker.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator
  routing
  traffic
  routing_common
  indexer
  platform
  geometry
  coding
  base
  gflags
  icu
  jansson
  minizip
  oauthcpp
  opening_hours
  protobuf
  stats_client
  succinct
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
