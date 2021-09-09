project(search_quality_tool)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(SRC search_quality_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} gflags::gflags)

omim_link_libraries(
  ${PROJECT_NAME}
  search_quality
  search_tests_support
  search
  storage
  editor
  indexer
  ge0
  platform
  mwm_diff
  bsdiff
  geometry
  coding
  base
  oauthcpp
  jansson
  protobuf
  minizip
  succinct
  opening_hours
  pugixml
  icu
  ${Qt5Core_LIBRARIES}
  ${Qt5Network_LIBRARIES}
  ${LIBZ}
)
