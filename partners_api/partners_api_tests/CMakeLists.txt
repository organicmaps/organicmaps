project(partners_api_tests)

add_definitions(-DOMIM_UNIT_TEST_WITH_QT_EVENT_LOOP)

set(
  SRC
  booking_tests.cpp
  uber_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  partners_api
  platform
  coding
  base
  tomcrypt
  jansson
  stats_client
  ${LIBZ}
  ${Qt5Core_LIBRARIES}
)

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
    "-framework IOKit"
    "-framework SystemConfiguration"
    "-framework Cocoa"
  )
endif()

if (PLATFORM_LINUX OR PLATFORM_WIN)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Network_LIBRARIES}
  )
endif()
