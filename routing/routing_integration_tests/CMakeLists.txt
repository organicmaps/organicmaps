# This subproject implements integration tests.
# This tests are launched on the whole world dataset.

# It is recommended to place tests here in the following cases:
# - tests are written to be launch on the whole world dataset;
# - tests covers significant number of subsystems;

project(routing_integration_tests)

include_directories(
        ${OMIM_ROOT}/3party/freetype/include
        ${OMIM_ROOT}/3party/glm
        ${OMIM_ROOT}/3party/jansson/src
        ${OMIM_ROOT}/3party/protobuf/protobuf/src
)

set(
  SRC
  absent_regions_finder_tests.cpp
  archival_reporter_tests.cpp
  bicycle_route_test.cpp
  bicycle_turn_test.cpp
  concurrent_feature_parsing_test.cpp
  cross_country_routing_tests.cpp
  get_altitude_test.cpp
  guides_tests.cpp
  pedestrian_route_test.cpp
  road_graph_tests.cpp
  roundabouts_tests.cpp
  route_test.cpp
  routing_test_tools.cpp
  routing_test_tools.hpp
  small_routes.cpp
  speed_camera_notifications_tests.cpp
  street_names_test.cpp
  transit_route_test.cpp
  turn_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  editor_tests_support
  generator
  map
  ge0
  tracking
  drape_frontend
  shaders
  routing
  search
  storage
  mwm_diff
  traffic
  routing_common
  transit
  descriptions
  ugc
  drape
  partners_api
  web_api
  local_ads
  platform_tests_support
  kml
  editor
  indexer
  metrics
  platform
  oauthcpp
  geometry
  coding
  base
  gflags
  jansson
  protobuf
  bsdiff
  succinct
  stats_client
  minizip
  pugixml
  opening_hours
  expat
  freetype
  icu
  sdf_image
  stb_image
  vulkan_wrapper
  agg
  ${LIBZ}
  ${Qt5Widgets_LIBRARIES}
)

if (PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    dl
  )
endif()

link_opengl(${PROJECT_NAME})
link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
