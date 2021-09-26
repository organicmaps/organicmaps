project(routes_builder_tool)

set(
  SRC
  routes_builder_tool.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} gflags::gflags)

omim_link_libraries(
  ${PROJECT_NAME}
  routes_builder
  routing_api
  routing
  traffic
  routing_common
  transit
  storage
  indexer
  platform
  mwm_diff
  bsdiff
  geometry
  coding
  base
  icu
  jansson
  oauthcpp
  opening_hours
  protobuf
  opening_hours
  succinct
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
