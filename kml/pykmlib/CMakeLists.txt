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
  platform
  coding
  geometry
  base
  expat
  stats_client
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
