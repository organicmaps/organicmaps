project(world_roads_builder)

set(
  SRC
  world_roads_builder.cpp
  world_roads_builder.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  ge0
  generator
  routing
  search
  storage
  mwm_diff
  bsdiff
  tracking
  traffic
  routing_common
  transit
  descriptions
  kml
  editor
  indexer
  platform
  geometry
  coding
  base
  freetype
  expat
  icu
  jansson
  protobuf
  minizip
  succinct
  pugixml
  oauthcpp
  opening_hours
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})

add_subdirectory(world_roads_builder_tool)
