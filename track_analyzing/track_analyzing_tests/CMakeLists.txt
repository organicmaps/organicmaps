project(track_analyzing_tests)

set(
  SRC
  ../track_analyzer/cmd_balance_csv.cpp
  ../track_analyzer/cmd_balance_csv.hpp
  ../track_analyzer/utils.cpp
  ../track_analyzer/utils.hpp
  balance_tests.cpp
  statistics_tests.cpp
  track_archive_reader_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  track_analyzing
  tracking
  generator
  routing
  traffic
  routing_common
  storage
  indexer
  platform
  minizip
  mwm_diff
  bsdiff
  geometry
  coding
  base
  icu
  jansson
  oauthcpp
  protobuf
  stats_client
  succinct
  gflags
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
