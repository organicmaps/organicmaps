project(routing_quality)

set(SRC
  waypoints.cpp
  waypoints.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  routes_builder
)

add_subdirectory(api)
omim_add_tool_subdirectory(routing_quality_tool)

omim_add_test_subdirectory(routing_quality_tests)
