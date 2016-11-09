# This subproject implements integration tests.
# This tests are launched on the whole world dataset.

# It is recommended to place tests here in the following cases:
# - tests are written to be launch on the whole world dataset;
# - tests covers significant number of subsystems;

project(routing_integration_tests)

set(
  SRC
  bicycle_route_test.cpp
  bicycle_turn_test.cpp
  cross_section_tests.cpp
  get_altitude_test.cpp
  online_cross_tests.cpp
  osrm_route_test.cpp
  osrm_street_names_test.cpp
  osrm_turn_test.cpp
  pedestrian_route_test.cpp
  routing_test_tools.cpp
  routing_test_tools.hpp
)

# if (APPLE)
#  target_compile_options(${PROJECT_NAME} PUBLIC "-fobjc-arc")
#endif()


omim_add_test(${PROJECT_NAME} ${SRC})
omim_link_libraries(
  ${PROJECT_NAME}
  map
  routing
  search
  storage
  indexer
  editor
  platform
  oauthcpp
  geometry
  coding
  base
  osrm
  jansson
  protobuf
  tomcrypt
  succinct
  stats_client
  pugixml
  opening_hours
  ${Qt5Core_LIBRARIES}
  ${LIBZ}
)

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    "-framework Cocoa"
    "-framework IOKit"
    "-framework SystemConfiguration"
  )
endif()
