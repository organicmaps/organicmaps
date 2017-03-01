project(openlr_stat)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  openlr_stat.cpp
)

add_executable(${PROJECT_NAME} ${SRC})
omim_link_libraries(${PROJECT_NAME}
  openlr
  routing
  routing_common
  indexer
  editor
  platform
  geometry
  coding
  base
  gflags
  jansson
  oauthcpp
  opening_hours
  protobuf
  pugixml
  stats_client
  ${Qt5Core_LIBRARIES}
  ${LIBZ}
)
