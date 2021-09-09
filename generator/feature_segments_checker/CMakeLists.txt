project(feature_segments_checker)

include_directories(
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  feature_segments_checker.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} gflags::gflags)

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
  icu
  jansson
  minizip
  oauthcpp
  opening_hours
  protobuf
  succinct
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
