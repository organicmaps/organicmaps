project(benchmark_tool)

include_directories(
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  api.cpp
  api.hpp
  features_loading.cpp
  main.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} gflags::gflags)

omim_link_libraries(
  ${PROJECT_NAME}
  map
  ge0
  editor
  indexer
  geometry
  platform
  coding
  base
  icu
  jansson
  oauthcpp
  opening_hours
  protobuf
  pugixml
  succinct
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
