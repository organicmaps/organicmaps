project(routes_builder_tool)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  routes_builder_tool.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

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
  stats_client
  succinct
  gflags
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
