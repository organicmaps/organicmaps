project(extract_addr)

include_directories(
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  extract_addr.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator
  search
  routing
  routing_common
  indexer
  editor
  geometry
  platform
  coding
  base
  minizip
  jansson
  pugixml
  stats_client
  opening_hours
  succinct
  oauthcpp
  expat
  protobuf
  icu
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
