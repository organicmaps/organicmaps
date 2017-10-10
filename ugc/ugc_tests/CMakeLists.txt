project(ugc_tests)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(
  SRC
  serdes_tests.cpp
  serdes_binary_tests.cpp
  storage_tests.cpp
  utils.cpp
  utils.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator_tests_support
  generator
  search
  routing
  routing_common
  ugc
  indexer
  storage
  editor
  platform
  coding
  geometry
  base
  icu
  jansson
  stats_client
  pugixml
  tess2
  protobuf
  succinct
  opening_hours
  oauthcpp
  ${LIBZ}
)
link_qt5_core(${PROJECT_NAME})
