project(qt_tstfrm)

set(
  SRC
  test_main_loop.hpp
  test_main_loop.cpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_compile_options(${PROJECT_NAME} PUBLIC -Wno-shorten-64-to-32 -Wno-deprecated-declarations)
