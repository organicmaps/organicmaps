project(pytrack_generator)

set(
  SRC
  bindings.cpp
)

include_directories(${CMAKE_BINARY_DIR})

omim_add_library(${PROJECT_NAME} MODULE ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  ${PYTHON_LIBRARIES}
  ${Boost_LIBRARIES}
  routing_quality
  routing
  routing_common
  coding
  geometry
  base
  stats_client
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
