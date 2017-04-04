project(local_ads)

set(
  SRC
  campaign.hpp
  campaign_serialization.cpp
  campaign_serialization.hpp
)

add_library(${PROJECT_NAME} ${SRC})

omim_add_pybindings_subdirectory(pylocal_ads)
omim_add_test_subdirectory(local_ads_tests)
