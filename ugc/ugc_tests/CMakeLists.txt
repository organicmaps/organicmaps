project(ugc_tests)

include_directories(${OMIM_ROOT}/3party/jansson/src)

option(UGC_MIGRATION "Build migration file for ugc" OFF)

set(
  SRC
  ratings_tests.cpp
  serdes_binary_tests.cpp
  serdes_tests.cpp
  storage_tests.cpp
  utils.cpp
  utils.hpp
)

if (UGC_MIGRATION)
  set(
    SRC
    ${SRC}
    migration/generate_migration_files.cpp
  )

  message("Generating files for UGC index migration")
endif()

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator_tests_support
  generator
  search
  routing
  routing_common
  ugc
  editor
  indexer
  storage
  platform
  platform_tests_support
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
  traffic
  ${CMAKE_DL_LIBS}
  ${LIBZ}
)
link_qt5_core(${PROJECT_NAME})
