project(openlr_tests)

set(
  SRC
  openlr_sample_test.cpp
  )

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  openlr
  indexer
  editor
  platform_tests_support
  platform
  coding
  geometry
  base
  jansson
  oauthcpp
  opening_hours
  pugixml
  stats_client
  ${Qt5Core_LIBRARIES}
  ${LIBZ}
)
