project(openlr_stat)

set(
  SRC
  openlr_stat.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} gflags::gflags)
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
  icu
  jansson
  oauthcpp
  opening_hours
  protobuf
  bsdiff
  pugixml
  succinct
  ${Qt5Core_LIBRARIES}
  ${LIBZ}
)

link_qt5_network(${PROJECT_NAME})
