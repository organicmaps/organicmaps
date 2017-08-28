project(partners_api)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(
  SRC
  ads_base.cpp
  ads_base.hpp
  ads_engine.cpp
  ads_engine.hpp
  banner.hpp
  booking_api.cpp
  booking_api.hpp
  cian_api.cpp
  cian_api.hpp
  facebook_ads.cpp
  facebook_ads.hpp
  google_ads.cpp
  google_ads.hpp
  locals_api.cpp
  locals_api.hpp
  mopub_ads.cpp
  mopub_ads.hpp
  opentable_api.cpp
  opentable_api.hpp
  rb_ads.cpp
  rb_ads.hpp
  taxi_base.cpp
  taxi_base.hpp
  taxi_countries.cpp
  taxi_countries.hpp
  taxi_engine.cpp
  taxi_engine.hpp
  taxi_places.hpp 
  taxi_provider.hpp
  uber_api.cpp
  uber_api.hpp
  utils.hpp
  viator_api.cpp
  viator_api.hpp
  yandex_api.cpp
  yandex_api.hpp
)

add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(partners_api_tests)
