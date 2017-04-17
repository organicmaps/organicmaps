project(local_ads)

set(
  SRC
  campaign.hpp
  campaign_serialization.cpp
  campaign_serialization.hpp
  config.hpp
  event.cpp
  event.hpp
  file_helpers.cpp
  file_helpers.hpp
  icons_info.cpp
  icons_info.hpp
  statistics.cpp
  statistics.hpp
)

add_library(${PROJECT_NAME} ${SRC})

omim_add_pybindings_subdirectory(pylocal_ads)
omim_add_test_subdirectory(local_ads_tests)
