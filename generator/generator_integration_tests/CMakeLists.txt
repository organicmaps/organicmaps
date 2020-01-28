project(generator_integration_tests)

set(
  SRC
  features_tests.cpp
  helpers.cpp
  helpers.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator_tests_support
  platform_tests_support
  generator
  drape_frontend
  shaders
  map
  ge0
  routing
  search
  storage
  drape
  traffic
  routing_common
  descriptions
  transit
  kml
  editor
  indexer
  platform
  geometry
  coding
  base
  freetype
  expat
  icu
  agg
  jansson
  protobuf
  stats_client
  minizip
  succinct
  pugixml
  tess2
  gflags
  oauthcpp
  opening_hours
  stb_image
  sdf_image
  vulkan_wrapper
  # TODO(syershov): Use FindPackage.
  sqlite3
  ${LIBZ}
)

if (PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    dl
  )
endif()

link_qt5_core(${PROJECT_NAME})
