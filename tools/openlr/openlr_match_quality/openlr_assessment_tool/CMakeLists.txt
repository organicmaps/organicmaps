project(openlr_assessment_tool)

set(SRC
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
  trafficmodeinitdlg.ui
)

omim_add_executable(${PROJECT_NAME} ${SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOUIC ON AUTOMOC ON)

target_link_libraries(${PROJECT_NAME}
  openlr
  qt_common
  map
  gflags::gflags
)
