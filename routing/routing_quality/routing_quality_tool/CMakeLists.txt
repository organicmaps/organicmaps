project(routing_quality_tool)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  benchmark_results.cpp
  benchmark_results.hpp
  benchmark_stat.cpp
  benchmark_stat.hpp
  error_type_counter.cpp
  error_type_counter.hpp
  routing_quality_tool.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  routing_quality
  routing_api
  routes_builder
  generator
  routing
  platform
  traffic
  routing_common
  transit
  storage
  indexer
  platform
  mwm_diff
  bsdiff
  kml
  geometry
  coding
  base
  icu
  jansson
  opening_hours
  oauthcpp
  protobuf
  stats_client
  gflags
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
