# This subproject implements integration tests.
# This tests are launched on the whole world dataset.

# It is recommended to place tests here in the following cases:
# - tests are written to be launch on the whole world dataset;
# - tests covers significant number of subsystems;

project(routing_integration_tests)

set(SRC
  absent_regions_finder_tests.cpp
  bicycle_route_test.cpp
  bicycle_turn_test.cpp
  concurrent_feature_parsing_test.cpp
  cross_country_routing_tests.cpp
  generated_map_test.cpp
  generated_map_test.hpp
  get_altitude_test.cpp
  guides_tests.cpp
  pedestrian_route_test.cpp
  road_graph_tests.cpp
  roundabouts_tests.cpp
  route_test.cpp
  routing_test_tools.cpp
  routing_test_tools.hpp
  samples_route_tests.cpp
  small_routes.cpp
  speed_camera_notifications_tests.cpp
  street_names_test.cpp
  transit_route_test.cpp
  turn_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC} REQUIRE_SERVER)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    generator_tests_support
    platform_tests_support
    routing
    map
)

# This regression needs no country maps, so it also runs with the unit test suites.
add_test(
  NAME transit_walk_distance_test
  COMMAND ${PROJECT_NAME}
    --filter=Transit_WalkDistanceIncludesGatesAndTransfers
    --data_path=${OMIM_DATA_DIR}
    --user_resource_path=${OMIM_USER_RESOURCES_DIR}
    --writable_path=${CMAKE_BINARY_DIR}/test_writable/transit_walk_distance_test
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(transit_walk_distance_test PROPERTIES LABELS "omim-test")
