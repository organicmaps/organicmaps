project(pysearch)

add_definitions(-DBOOST_BIND_NO_PLACEHOLDERS)

set(
  SRC
  bindings.cpp
)

add_library(${PROJECT_NAME} MODULE ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  search_tests_support
  search
  storage
  indexer
  editor
  geometry
  platform
  mwm_diff
  coding
  base
  bsdiff
  icu
  jansson
  oauthcpp
  opening_hours
  protobuf
  pugixml
  stats_client
  succinct
  ${PYTHON_LIBRARIES}
  ${Boost_LIBRARIES}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
