project(aloha_to_samples_tool)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(SRC aloha_to_samples_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

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
  gflags
  jansson
  protobuf
  stats_client
  minizip
  succinct
  opening_hours
  pugixml
  icu
  ${Qt5Core_LIBRARIES}
  ${Qt5Network_LIBRARIES}
  ${LIBZ}
)
