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
  traffic
  routing_common
  indexer
  editor
  platform_tests_support
  platform
  coding
  geometry
  base
  icu
  jansson
  oauthcpp
  opening_hours
  protobuf
  pugixml
  stats_client
  ${LIBZ}
)

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
  )
endif()

if (PLATFORM_WIN OR PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Network_LIBRARIES}
  )
endif()

link_qt5_core(${PROJECT_NAME})
