include_directories(${OMIM_ROOT}/3party/jansson/src)

set(
  SRC
  algos_tests.cpp
  house_detector_tests.cpp
  house_numbers_matcher_test.cpp
  interval_set_test.cpp
  keyword_lang_matcher_test.cpp
  keyword_matcher_test.cpp
  latlon_match_test.cpp
  locality_finder_test.cpp
  locality_scorer_test.cpp
  locality_selector_test.cpp
  match_cost_mock.hpp
  nearby_points_sweeper_test.cpp
  query_saver_tests.cpp
  ranking_tests.cpp
  string_intersection_test.cpp
  string_match_test.cpp
)

omim_add_test(search_tests ${SRC})

omim_link_libraries(
  search_tests
  search
  indexer
  editor
  oauthcpp
  platform
  opening_hours
  geometry
  coding
  base
  protobuf
  jansson
  tomcrypt
  succinct
  pugixml
  stats_client
  ${Qt5Core_LIBRARIES}
  ${LIBZ}
)

if (PLATFORM_MAC)
  omim_link_libraries(
    search_tests
    "-framework Cocoa"
    "-framework IOKit"
    "-framework SystemConfiguration"
  )
endif ()
