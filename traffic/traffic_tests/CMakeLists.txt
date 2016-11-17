project(traffic_tests)

add_definitions(-DOMIM_UNIT_TEST_WITH_QT_EVENT_LOOP)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(
  SRC
  traffic_info_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  routing
  traffic
  indexer
  platform_tests_support
  platform
  coding
  geometry
  base
  stats_client
  tomcrypt
  ${Qt5Core_LIBRARIES}
)

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
    "-framework IOKit"
    "-framework Cocoa"
    "-framework SystemConfiguration"
  )
endif()

if (PLATFORM_WIN OR PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Network_LIBRARIES}
  )
endif()
