project(openlr)

set(SRC
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
  router.cpp
  router.hpp
  score_candidate_paths_getter.cpp
  score_candidate_paths_getter.hpp
  score_candidate_points_getter.cpp
  score_candidate_points_getter.hpp
  score_paths_connector.cpp
  score_paths_connector.hpp
  score_types.hpp
  stats.hpp
  way_point.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  routing
  indexer
  geometry
  pugixml
)

omim_add_tool_subdirectory(openlr_match_quality)
omim_add_tool_subdirectory(openlr_stat)

omim_add_test_subdirectory(openlr_tests)
