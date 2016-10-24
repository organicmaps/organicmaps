project(openlr_stat)

set(
  SRC
  openlr_stat.cpp
)

add_executable(${PROJECT_NAME} ${SRC})
omim_link_libraries(${PROJECT_NAME}
  openlr
  routing
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
