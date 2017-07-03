project(ugc_tests)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(
  SRC
  serdes_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})
omim_link_libraries(
  ${PROJECT_NAME}
  ugc
  indexer
  platform
  coding
  geometry
  base
  jansson
  stats_client
  ${LIBZ}
)
link_qt5_core(${PROJECT_NAME})
