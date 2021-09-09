project(extrapolation_benchmark)

include_directories(
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  extrapolation_benchmark.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} gflags::gflags)

omim_link_libraries(
  ${PROJECT_NAME}
  map
  ge0
  routing
  platform
  geometry
  coding
  base
  oauthcpp
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
