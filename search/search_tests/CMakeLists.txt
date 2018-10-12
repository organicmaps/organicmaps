project(search_tests)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(
  SRC
  algos_tests.cpp
  bookmarks_processor_tests.cpp
  highlighting_tests.cpp
  house_detector_tests.cpp
  house_numbers_matcher_test.cpp
  interval_set_test.cpp
  keyword_lang_matcher_test.cpp
  keyword_matcher_test.cpp
  latlon_match_test.cpp
  localities_source_tests.cpp
  locality_finder_test.cpp
  locality_scorer_test.cpp
  locality_selector_test.cpp
  mem_search_index_tests.cpp
  point_rect_matcher_tests.cpp
  query_saver_tests.cpp
  ranking_tests.cpp
  results_tests.cpp
  region_info_getter_tests.cpp
  segment_tree_tests.cpp
  string_match_test.cpp
  text_index_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  search_tests_support
  generator_tests_support
  platform_tests_support
  search
  editor
  indexer
  storage
  platform
  geometry
  coding
  base
  jansson
  oauthcpp
  opening_hours
  protobuf
  pugixml
  stats_client
  succinct
  icu
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
