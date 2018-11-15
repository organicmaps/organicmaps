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
  routing
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
  protobuf
  stats_client
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
