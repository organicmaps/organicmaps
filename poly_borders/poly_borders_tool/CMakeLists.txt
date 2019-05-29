project(poly_borders_tool)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(SRC poly_borders_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  poly_borders
  generator
  routing
  traffic
  routing_common
  descriptions
  transit
  ugc
  search
  storage
  editor
  indexer
  mwm_diff
  platform
  geometry
  coding
  base
  pugixml
  opening_hours
  icu
  jansson
  protobuf
  stats_client
  succinct
  tess2
  gflags
  oauthcpp
  sqlite3
  ${CMAKE_DL_LIBS}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
