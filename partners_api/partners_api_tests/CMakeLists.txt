project(partners_api_tests)

add_definitions(-DOMIM_UNIT_TEST_WITH_QT_EVENT_LOOP)

set(
  SRC
  ads_engine_tests.cpp
  arsenal_tests.cpp
  booking_tests.cpp
  bookmark_catalog_ads_tests.cpp
  citymobil_tests.cpp
  citymobil_ads_tests.cpp
  download_on_map_container_delegate.hpp
  facebook_tests.cpp
  freenow_tests.cpp
  google_tests.cpp
  guides_on_map_tests.cpp
  mastercard_sberbank_tests.cpp
# Maxim taxi project is disabled.
# maxim_tests.cpp
# Megafon project is disabled until the contract is renewed or extended.
# megafon_countries_tests.cpp
  mopub_tests.cpp
  mts_tests.cpp
  promo_tests.cpp
  rb_tests.cpp
  rutaxi_tests.cpp
  skyeng_tests.cpp
  taxi_engine_tests.cpp
  taxi_places_tests.cpp
  tinkoff_airlines_tests.cpp
  tinkoff_insurance_tests.cpp
# uber_tests.cpp
  yandex_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  platform_tests_support
  metrics_tests_support
  partners_api
  web_api
  metrics
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
