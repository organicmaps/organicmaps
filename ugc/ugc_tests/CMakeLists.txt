project(ugc_tests)

set(
  SRC
  serdes_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})
omim_link_libraries(${PROJECT_NAME} ugc indexer platform coding geometry base)
link_qt5_core(${PROJECT_NAME})
