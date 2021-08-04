# Console utility to convert KMB file to KML.
project(kmb_to_kml)

set(SRC
  kmb_to_kml.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(${PROJECT_NAME}
  kml
  indexer
  platform
  geometry
  coding
  base
  oauthcpp
  protobuf
  succinct
)

link_qt5_core(${PROJECT_NAME})
