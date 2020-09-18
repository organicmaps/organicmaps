project(track_analyzing)

set(
  SRC
  exceptions.hpp
  log_parser.cpp
  log_parser.hpp
  serialization.hpp
  temporary_file.cpp
  temporary_file.hpp
  track.cpp
  track.hpp
  track_archive_reader.cpp
  track_archive_reader.hpp
  track_matcher.cpp
  track_matcher.hpp
  utils.cpp
  utils.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
add_subdirectory(track_analyzer)
add_subdirectory(track_analyzing_tests)
