# This subproject implements routing consistency tests.
# This tests are launched on the whole world dataset.

project(routing_consistency_tests)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  ../routing_integration_tests/routing_test_tools.cpp
  routing_consistency_tests.cpp
)

# Not using omim_add_test because we don't need testingmain.cpp
add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  map
  routing
  traffic
  routing_common
  search
  storage
  indexer
  platform
  editor
  geometry
  oauthcpp
  opening_hours
  coding
  base
  osrm
  jansson
  protobuf
  succinct
  stats_client
  generator
  gflags
  pugixml
  ${Qt5Widgets_LIBRARIES}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
