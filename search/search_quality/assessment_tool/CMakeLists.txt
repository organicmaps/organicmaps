project(assessment_tool)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

include_directories(${OMIM_ROOT}/3party/gflags/src)
include_directories(${OMIM_ROOT}/3party/glm)

set(
  SRC
  assessment_tool.cc
  main_model.cpp
  main_model.hpp
  main_model.hpp
  main_view.cpp
  main_view.hpp
  model.hpp
  view.hpp
)

add_executable(${PROJECT_NAME} ${SRC})

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
  platform
  editor geometry
  coding
  base
  expat
  freetype
  fribidi
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
