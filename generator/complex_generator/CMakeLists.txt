project(complex_generator)

set(SRC complex_generator.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} gflags::gflags)

omim_link_libraries(
  ${PROJECT_NAME}
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
  opening_hours
  freetype
  expat
  icu
  jansson
  protobuf
  bsdiff
  minizip
  succinct
  pugixml
  tess2
  oauthcpp
  sqlite3
  ${CMAKE_DL_LIBS}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
