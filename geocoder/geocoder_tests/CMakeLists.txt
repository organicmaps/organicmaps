project(geocoder_tests)

set(
  SRC
  geocoder_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  platform_tests_support
  geocoder
  search
  indexer
  platform
  coding
  base
  stats_client
  jansson
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
