project(restaurants_info)

include_directories(
  ${OMIM_ROOT}/3party/gflags/src
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  restaurants_info.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator
  search
  routing
  routing_common
  indexer
  geometry
  editor
  platform
  coding
  base
  minizip
  jansson
  pugixml
  stats_client
  opening_hours
  succinct
  gflags
  oauthcpp
  expat
  protobuf
  icu
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
