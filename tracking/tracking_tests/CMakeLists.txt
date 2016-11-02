set(
  SRC
  protocol_test.cpp
  reporter_test.cpp
)

omim_add_test(tracking_tests ${SRC})
omim_link_libraries(tracking_tests tracking platform platform_tests_support coding base geometry tomcrypt ${Qt5Widgets_LIBRARIES})

if (${PLATFORM_MAC})
  target_link_libraries(tracking_tests
    "-framework Cocoa"
    "-framework IOKit"
    "-framework SystemConfiguration"
  )
endif()
