project(generator_tool)

include_directories(${OMIM_PATH}/3party/gflags/src)

set(SRC generator_tool.cpp)

add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator
  routing
  traffic
  routing_common
  search
  storage
  indexer
  editor
  platform
  geometry
  coding
  base
  opening_hours
  freetype
  expat
  fribidi
  jansson
  protobuf
  osrm
  stats_client
  minizip
  succinct
  pugixml
  tess2
  gflags
  oauthcpp
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
