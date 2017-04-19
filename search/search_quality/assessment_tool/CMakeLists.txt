project(assessment_tool)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

include_directories(${OMIM_ROOT}/3party/gflags/src)
include_directories(${OMIM_ROOT}/3party/glm)

set(
  SRC
  assessment_tool.cpp
  context.cpp
  context.hpp
  edits.cpp
  edits.hpp
  helpers.cpp
  helpers.hpp
  languages_list.cpp
  languages_list.hpp
  main_model.cpp
  main_model.hpp
  main_view.cpp
  main_view.hpp
  model.hpp
  result_view.cpp
  result_view.hpp
  results_view.cpp
  results_view.hpp
  sample_view.cpp
  sample_view.hpp
  samples_view.cpp
  samples_view.hpp
  view.hpp
)

omim_add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  qt_common
  map
  drape_frontend
  routing
  search_quality
  search
  storage
  tracking
  traffic
  routing_common
  indexer
  drape
  partners_api
  local_ads
  platform
  editor geometry
  coding
  base
  expat
  freetype
  icu
  gflags
  jansson
  minizip
  oauthcpp
  opening_hours
  openlr
  osrm
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
