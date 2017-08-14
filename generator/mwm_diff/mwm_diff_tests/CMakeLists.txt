project(mwm_diff_tests)

set(
  SRC
  diff_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  mwm_diff
  bsdiff
  platform
  coding
  base
  stats_client
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
