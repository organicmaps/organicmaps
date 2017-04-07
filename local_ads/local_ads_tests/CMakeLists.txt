project(local_ads_tests)

set(
  SRC
  campaign_serialization_test.cpp
  file_helpers_tests.cpp
  statistics_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  local_ads
  coding
  geometry
  platform
  platform_tests_support
  base
  stats_client
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
