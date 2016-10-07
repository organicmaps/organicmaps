add_definitions(-DOMIM_UNIT_TEST_WITH_QT_EVENT_LOOP)
include_directories(${APL_SYSCON})

set(
  SRC
  apk_test.cpp
  country_file_tests.cpp
  get_text_by_id_tests.cpp
  jansson_test.cpp
  language_test.cpp
  local_country_file_tests.cpp
  location_test.cpp
  measurement_tests.cpp
  mwm_version_test.cpp
  platform_test.cpp
)

omim_add_test(platform_tests ${SRC})
omim_link_libraries(platform_tests platform_tests_support platform coding base jansson minizip tomcrypt ${LIBZ})


if (${PLATFORM_MAC})
  target_link_libraries(platform_tests ${Qt5Widgets_LIBRARIES}
    "-framework Cocoa"
    "-framework IOKit"
    "-framework QuartzCore"
    "-framework SystemConfiguration"
  )
endif()

if (${PLATFORM_WIN} OR ${PLATFORM_LINUX})
  target_link_libraries(platform_tests ${Qt5Network_LIBRARIES})
endif()
