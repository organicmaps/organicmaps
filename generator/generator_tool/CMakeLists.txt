project(generator_tool)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(SRC generator_tool.cpp)

if (BUILD_DESIGNER)
  set(SRC MACOSX_BUNDLE ${SRC})
endif()

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
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
  mwm_diff
  bsdiff
  stats_client
  minizip
  succinct
  pugixml
  tess2
  gflags
  oauthcpp
  sqlite3
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
