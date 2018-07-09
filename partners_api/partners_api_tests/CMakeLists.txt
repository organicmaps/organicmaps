project(partners_api_tests)

add_definitions(-DOMIM_UNIT_TEST_WITH_QT_EVENT_LOOP)

set(
  SRC
  ads_engine_tests.cpp
  booking_tests.cpp
  facebook_tests.cpp
  google_tests.cpp
  maxim_tests.cpp
  megafon_countries_tests.cpp
  mopub_tests.cpp
  rb_tests.cpp
  taxi_engine_tests.cpp
  taxi_places_tests.cpp
  uber_tests.cpp
  viator_tests.cpp
  yandex_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  partners_api
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
  stats_client
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
