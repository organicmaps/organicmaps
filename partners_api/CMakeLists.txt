project(partners_api)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(
  SRC
  booking_api.hpp
  booking_api.cpp
  facebook_ads.hpp
  facebook_ads.cpp
  opentable_api.hpp
  opentable_api.cpp
  uber_api.hpp
  uber_api.cpp
)

add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(partners_api_tests)
