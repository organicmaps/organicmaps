project(world_roads_builder)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  world_roads_builder.cpp
  world_roads_builder.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  ge0
  drape_frontend
  shaders
  generator
  routing
  search
  storage
  mwm_diff
  bsdiff
  tracking
  traffic
  routing_common
  transit
  descriptions
  ugc
  drape
  partners_api
  web_api
  local_ads
  kml
  editor
  indexer
  metrics
  platform
  geometry
  coding
  base
  freetype
  expat
  icu
  agg
  jansson
  protobuf
  stats_client
  minizip
  succinct
  pugixml
  oauthcpp
  opening_hours
  stb_image
  sdf_image
  vulkan_wrapper
  ${LIBZ}
)

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
)
endif()

if (PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    dl
)
endif()

link_opengl(${PROJECT_NAME})
link_qt5_core(${PROJECT_NAME})

add_subdirectory(world_roads_builder_tool)
