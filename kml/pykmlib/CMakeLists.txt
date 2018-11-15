project(pykmlib)

set(
  SRC
  bindings.cpp
)

omim_add_library(${PROJECT_NAME} MODULE ${SRC})

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
  oauthcpp
  protobuf
  pugixml
  expat
  stats_client
)

link_qt5_core(${PROJECT_NAME})

if (PLATFORM_MAC)
  omim_link_libraries(${PROJECT_NAME} ${LIBZ})
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
