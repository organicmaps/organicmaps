project(pykmlib)

set(
  SRC
  bindings.cpp
)

add_library(${PROJECT_NAME} MODULE ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  ${PYTHON_LIBRARIES}
  ${Boost_LIBRARIES}
  kml
  indexer
  editor
  platform
  coding
  geometry
  base
  icu
  jansson
  protobuf
  pugixml
  expat
  stats_client
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
