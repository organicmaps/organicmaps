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
  facebook_ads.cpp
  facebook_ads.hpp
  mopub_ads.cpp
  mopub_ads.hpp
  opentable_api.cpp
  opentable_api.hpp
  rb_ads.cpp
  rb_ads.hpp
  uber_api.cpp
  uber_api.hpp
)

add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(partners_api_tests)
