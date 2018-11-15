project(style_tests)

include_directories(${OMIM_ROOT}/3party/protobuf/protobuf/src)

set(
  SRC
  classificator_tests.cpp
  dashes_test.cpp
  helpers.hpp
  style_symbols_consistency_test.cpp
)


if (BUILD_DESIGNER)
  set(SRC MACOSX_BUNDLE ${SRC})
endif()

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  indexer
  platform
  geometry
  coding
  base
  expat
  icu
  jansson
  oauthcpp
  protobuf
  stats_client
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
