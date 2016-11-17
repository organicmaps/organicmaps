project(mwm_tests)

add_compile_options(
  "-Wall"
  "-std=c++11"
)

set(
  SRC
  multithread_mwm_test.cpp
  mwm_foreach_test.cpp
  mwm_index_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})
omim_link_libraries(
  ${PROJECT_NAME}
  map
  search
  storage
  indexer
  editor
  platform
  geometry
  coding
  base
  opening_hours
  freetype
  fribidi
  expat
  oauthcpp
  protobuf
  jansson
  succinct
  pugixml
  stats_client
  ${Qt5Core_LIBRARIES}
  ${Qt5OpenGL_LIBRARIES}
  ${LIBZ}
)

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    "-framework Cocoa"
    "-framework IOKit"
    "-framework QuartzCore"
    "-framework SystemConfiguration"
  )
endif()
