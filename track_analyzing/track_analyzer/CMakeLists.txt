project(track_analyzer)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  cmd_cpp_track.cpp
  cmd_match.cpp
  cmd_table.cpp
  cmd_track.cpp
  cmd_tracks.cpp
  track_analyzer.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  track_analyzing
  generator
  map
  routing
  traffic
  routing_common
  storage
  indexer
  editor
  platform
  geometry
  coding
  base
  opening_hours
  icu
  jansson
  protobuf
  osrm
  stats_client
  succinct
  pugixml
  gflags
  oauthcpp
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
