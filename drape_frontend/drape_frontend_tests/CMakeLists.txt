project(drape_frontend_tests)

set(
  SRC
  navigator_test.cpp
  object_pool_tests.cpp
  user_event_stream_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
    "-framework Cocoa"
    "-framework IOKit"
    "-framework QuartzCore"
    "-framework SystemConfiguration"
  )
endif()

omim_link_libraries(
  ${PROJECT_NAME}
  drape_frontend
  drape
  platform
  indexer
  geometry
  coding
  base
  expat
  tomcrypt
  stats_client
  ${Qt5OpenGL_LIBRARIES}
)