project(track_analyzer)

set(SRC
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

target_link_libraries(${PROJECT_NAME}
  map
  track_analyzing
  gflags::gflags
)
