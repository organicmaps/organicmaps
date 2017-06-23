project(ugc_tests)

set(
  SRC
  serdes_tests.cpp
  serdes_json_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})
omim_link_libraries(
  ${PROJECT_NAME}
  ugc
  jansson
  indexer
  platform
  coding
  geometry
  base
  stats_client
  ${LIBZ}
)
link_qt5_core(${PROJECT_NAME})
