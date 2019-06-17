project(routes_builder)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  data_source_storage.cpp
  data_source_storage.hpp
  routes_builder.cpp
  routes_builder.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

add_subdirectory(routes_builder_tool)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
