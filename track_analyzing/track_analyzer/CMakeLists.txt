project(track_analyzer)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  cmd_balance_csv.cpp
  cmd_balance_csv.hpp
  cmd_cpp_track.cpp
  cmd_gpx.cpp
  cmd_match.cpp
  cmd_table.cpp
  cmd_track.cpp
  cmd_tracks.cpp
  cmd_unmatched_tracks.cpp
  crossroad_checker.cpp
  crossroad_checker.hpp
  track_analyzer.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  track_analyzing
  generator
  routing
  traffic
  routing_common
  storage
  indexer
  platform
  mwm_diff
  bsdiff
  geometry
  coding
  base
  icu
  jansson
  oauthcpp
  opening_hours
  protobuf
  stats_client
  succinct
  gflags
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
