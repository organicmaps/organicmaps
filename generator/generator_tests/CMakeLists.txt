project(generator_tests)

set(
  SRC
  altitude_test.cpp
  check_mwms.cpp
  coasts_test.cpp
  feature_builder_test.cpp
  feature_merger_test.cpp
  metadata_parser_test.cpp
  osm2meta_test.cpp
  osm_id_test.cpp
  osm_o5m_source_test.cpp
  osm_type_test.cpp
  restriction_collector_test.cpp
  restriction_test.cpp
  source_data.cpp
  source_data.hpp
  source_to_element_test.cpp
  srtm_parser_test.cpp
  tag_admixer_test.cpp
  tesselator_test.cpp
  triangles_tree_coding_test.cpp
  types_helper.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator_tests_support
  platform_tests_support
  generator
  drape_frontend
  routing
  search
  storage
  indexer
  drape
  map
  traffic
  editor
  oauthcpp
  platform
  geometry
  coding
  base
  freetype
  expat
  fribidi
  jansson
  protobuf
  osrm
  stats_client
  minizip
  succinct
  pugixml
  tess2
  gflags
  opening_hours
  ${LIBZ}
  ${Qt5Core_LIBRARIES}
)

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    "-framework IOKit"
    "-framework QuartzCore"
    "-framework Cocoa"
    "-framework SystemConfiguration"
  )
endif()
