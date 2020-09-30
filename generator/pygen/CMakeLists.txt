project(pygen)

set(
  SRC
  pygen.cpp
)

include_directories(${CMAKE_BINARY_DIR})

omim_add_library(${PROJECT_NAME} MODULE ${SRC})

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
  stats_client
  minizip
  succinct
  pugixml
  tess2
  gflags
  oauthcpp
  sqlite3
  ${CMAKE_DL_LIBS}
  ${LIBZ}
  ${Boost_LIBRARIES}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})

if (PLATFORM_MAC)
  omim_link_libraries(${PROJECT_NAME} "-Wl,-undefined,dynamic_lookup")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
