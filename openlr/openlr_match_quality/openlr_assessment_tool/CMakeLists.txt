project(openlr_assessment_tool)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

include_directories(
  ${OMIM_ROOT}/3party/gflags/src
  ${OMIM_ROOT}/3party/glm
  ${CMAKE_CURRENT_BINARY_DIR}
)

set(
  SRC
  main.cpp
  mainwindow.cpp
  mainwindow.hpp
  map_widget.cpp
  map_widget.hpp
  points_controller_delegate_base.hpp
  segment_correspondence.cpp
  segment_correspondence.hpp
  traffic_drawer_delegate_base.hpp
  traffic_mode.cpp
  traffic_mode.hpp
  traffic_panel.cpp
  traffic_panel.hpp
  trafficmodeinitdlg.cpp
  trafficmodeinitdlg.h
)

omim_add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  qt_common
  map
  drape_frontend
  shaders
  routing
  search_quality
  search
  storage
  tracking
  traffic
  routing_common
  transit
  descriptions
  ugc
  drape
  partners_api
  local_ads
  kml
  editor
  indexer
  metrics
  platform
  mwm_diff
  bsdiff
  geometry
  coding
  base
  expat
  freetype
  icu
  agg
  gflags
  jansson
  minizip
  oauthcpp
  opening_hours
  openlr
  protobuf
  pugixml
  sdf_image
  stats_client
  stb_image
  succinct
  ${Qt5Widgets_LIBRARIES}
  ${LIBZ}
)

link_opengl(${PROJECT_NAME})
link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})

if (PLATFORM_MAC)
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/Info.plist
  )
endif()
