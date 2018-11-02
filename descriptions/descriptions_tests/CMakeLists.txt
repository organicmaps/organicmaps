project(descriptions_tests)

set(
  SRC
  descriptions_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  descriptions
  indexer
  platform
  coding
  base
  jansson
  stats_client
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
