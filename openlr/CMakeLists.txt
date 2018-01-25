project(openlr)

set(
  SRC
  cache_line_size.hpp
  candidate_paths_getter.cpp
  candidate_paths_getter.hpp
  candidate_points_getter.cpp
  candidate_points_getter.hpp
  decoded_path.cpp
  decoded_path.hpp
  graph.cpp
  graph.hpp
  helpers.cpp
  helpers.hpp
  openlr_decoder.cpp
  openlr_decoder.hpp
  openlr_model.cpp
  openlr_model.hpp
  openlr_model_xml.cpp
  openlr_model_xml.hpp
  paths_connector.cpp
  paths_connector.hpp
  road_info_getter.cpp
  road_info_getter.hpp
  road_type_checkers.cpp
  road_type_checkers.hpp
  router.cpp
  router.hpp
  stats.hpp
  way_point.hpp
)

add_library(${PROJECT_NAME} ${SRC})
add_subdirectory(openlr_match_quality)
add_subdirectory(openlr_stat)
omim_add_test_subdirectory(openlr_tests)
