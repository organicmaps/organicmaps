project(qt_tstfrm)

set(SRC
  test_main_loop.hpp
  test_main_loop.cpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  Qt6::Core
  Qt6::Widgets
)
