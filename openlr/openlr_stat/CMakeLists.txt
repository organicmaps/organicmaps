project(openlr_stat)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  openlr_stat.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})
omim_link_libraries(${PROJECT_NAME}
  openlr
  routing
  routing_common
  storage
  editor
  indexer
  platform
  geometry
  coding
  base
  gflags
  icu
  jansson
  oauthcpp
  opening_hours
  protobuf
  mwm_diff
  bsdiff
  pugixml
  stats_client
  ${Qt5Core_LIBRARIES}
  ${LIBZ}
)

link_qt5_network(${PROJECT_NAME})
