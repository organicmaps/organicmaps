project(drape_frontend_tests)

set(
  SRC
  frame_values_tests.cpp
  navigator_test.cpp
  path_text_test.cpp
  user_event_stream_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
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
  stats_client
  oauthcpp
  freetype
  stb_image
  sdf_image
  icu
  vulkan_wrapper
  ${LIBZ}
)

if (PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    dl
  )
endif()

link_opengl(${PROJECT_NAME})
link_qt5_core(${PROJECT_NAME})
