project(partners_api_tests)

add_definitions(-DOMIM_UNIT_TEST_WITH_QT_EVENT_LOOP)

set(
  SRC
  booking_tests.cpp
  citymobil_tests.cpp
  freenow_tests.cpp
  guides_on_map_tests.cpp
# Maxim taxi project is disabled.
# maxim_tests.cpp
  rutaxi_tests.cpp
  taxi_engine_tests.cpp
  taxi_places_tests.cpp
# uber_tests.cpp
  yandex_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  platform_tests_support
  partners_api
  web_api
  storage
  indexer
  platform
  editor
  mwm_diff
  bsdiff
  geometry 
  coding
  base
  jansson
  succinct
  pugixml
  protobuf
  oauthcpp
  opening_hours
  icu
  ${LIBZ}
  ${Qt5Widgets_LIBRARIES}
  ${Qt5Network_LIBRARIES}
)

link_qt5_core(${PROJECT_NAME})
