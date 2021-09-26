# This subproject implements routing consistency tests.
# This tests are launched on the whole world dataset.

project(routing_consistency_tests)

set(
  SRC
  ../routing_integration_tests/routing_test_tools.cpp
  routing_consistency_tests.cpp
)

# Not using omim_add_test because we don't need testingmain.cpp
omim_add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} gflags::gflags)

omim_link_libraries(
  ${PROJECT_NAME}
  map
  ge0
  routing
  traffic
  routing_common
  transit
  search
  storage
  mwm_diff
  kml
  editor
  indexer
  platform
  geometry
  oauthcpp
  opening_hours
  coding
  base
  jansson
  protobuf
  bsdiff
  succinct
  generator
  pugixml
  icu
  agg
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
