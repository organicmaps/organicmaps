project(routing_quality_tests)

set(SRC
  barriers_tests.cpp
  bigger_roads_tests.cpp
  ferry_tests.cpp
  is_built_tests.cpp
  leaps_postprocessing_tests.cpp
  passby_roads_tests.cpp
  waypoints_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  routing_quality
)
