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

omim_add_test(mwm_tests ${SRC})
omim_link_libraries(
  mwm_tests
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
  tomcrypt
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
    mwm_tests
    "-framework Cocoa"
    "-framework IOKit"
    "-framework QuartzCore"
    "-framework SystemConfiguration"
  )
endif()
