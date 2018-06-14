project(pytraffic)

set(
  SRC
  bindings.cpp
)

omim_add_library(${PROJECT_NAME} MODULE ${SRC})

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
    "-framework QuartzCore"
  )
endif()

if (PLATFORM_WIN OR PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
  )
endif()

omim_link_libraries(
  ${PROJECT_NAME}
  routing
  traffic
  routing_common
  indexer
  editor
  platform
  geometry
  coding
  base
  stats_client
  jansson
  oauthcpp
  protobuf
  pugixml
  opening_hours
  icu
  ${PYTHON_LIBRARIES}
  ${Boost_LIBRARIES}
  ${LIBZ}
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
