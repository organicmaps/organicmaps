project(routing_tests)

include_directories(${OMIM_ROOT}/3party/osrm/osrm-backend/include)

set(
  SRC
  astar_algorithm_test.cpp
  astar_progress_test.cpp
  astar_router_test.cpp
  async_router_test.cpp
  cross_routing_tests.cpp
  followed_polyline_test.cpp
  nearest_edge_finder_tests.cpp
  online_cross_fetcher_test.cpp
  osrm_router_test.cpp
  road_graph_builder.cpp
  road_graph_builder.hpp
  road_graph_nearest_edges_test.cpp
  route_tests.cpp
  routing_mapping_test.cpp
  routing_session_test.cpp
  turns_generator_test.cpp
  turns_sound_test.cpp
  turns_tts_text_tests.cpp
  vehicle_model_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})
omim_link_libraries(
  ${PROJECT_NAME}
  routing
  indexer
  platform_tests_support
  editor
  platform
  oauthcpp
  geometry
  coding
  base
  osrm
  protobuf
  tomcrypt
  succinct
  opening_hours
  jansson
  stats_client
  map
  pugixml
  stats_client
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