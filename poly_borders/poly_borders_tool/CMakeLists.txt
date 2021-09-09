project(poly_borders_tool)

set(SRC poly_borders_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} gflags::gflags)

omim_link_libraries(
  ${PROJECT_NAME}
  poly_borders
  generator
  routing
  traffic
  routing_common
  descriptions
  transit
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
  succinct
  tess2
  oauthcpp
  sqlite3
  ${CMAKE_DL_LIBS}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
