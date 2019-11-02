project(bsdiff_tests)

set(
  SRC
  bsdiff_search_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  bsdiff
  platform
  coding
  base
  stats_client
  oauthcpp
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
