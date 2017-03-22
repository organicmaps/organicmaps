project(search_quality_tests)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(SRC sample_test.cpp)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  search_quality
  platform
  coding
  base
  jansson
  stats_client
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
