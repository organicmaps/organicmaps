project(tracking_fuzz_tests)

set(
  SRC
  tracking_fuzz_tests.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  tracking
  coding
  base
  geometry
)

link_qt5_core(${PROJECT_NAME})
