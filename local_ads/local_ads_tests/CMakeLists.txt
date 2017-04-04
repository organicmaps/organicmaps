project(local_ads_tests)

set(
  SRC
  campaign_serialization_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  local_ads
  coding
  platform
  base
  stats_client
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
