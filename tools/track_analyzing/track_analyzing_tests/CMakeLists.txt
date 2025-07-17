project(track_analyzing_tests)

set(SRC
  ../track_analyzer/cmd_balance_csv.cpp
  ../track_analyzer/cmd_balance_csv.hpp
  ../track_analyzer/utils.cpp
  ../track_analyzer/utils.hpp
  balance_tests.cpp
  statistics_tests.cpp
  track_archive_reader_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  map
  tracking
  track_analyzing
)
