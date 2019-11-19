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
  traffic
  routing_common
  storage
  editor
  indexer
  mwm_diff
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
  bsdiff
  pugixml
  stats_client
  succinct
  ${Qt5Core_LIBRARIES}
  ${LIBZ}
)

link_qt5_network(${PROJECT_NAME})
