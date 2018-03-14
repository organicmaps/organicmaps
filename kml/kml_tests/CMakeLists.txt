project(kml_tests)

set(
  SRC
  serdes_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  kml
  platform
  coding
  geometry
  base
  expat
  stats_client
  ${LIBZ}
)
link_qt5_core(${PROJECT_NAME})
