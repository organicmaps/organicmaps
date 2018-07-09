project(qt_tstfrm)

set(
  SRC
  test_main_loop.hpp
  test_main_loop.cpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
