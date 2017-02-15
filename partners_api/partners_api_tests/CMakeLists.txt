project(partners_api_tests)

add_definitions(-DOMIM_UNIT_TEST_WITH_QT_EVENT_LOOP)

set(
  SRC
  booking_tests.cpp
  facebook_tests.cpp
  uber_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  partners_api
  platform
  geometry
  coding
  editor
  base
  indexer
  jansson
  stats_client
  pugixml
  protobuf
  oauthcpp
  opening_hours
  ${LIBZ}
  ${Qt5Widgets_LIBRARIES}
  ${Qt5Network_LIBRARIES}
)

link_qt5_core(${PROJECT_NAME})
