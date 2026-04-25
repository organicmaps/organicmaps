project(qt_common_renderer_base)

set(SRC
  mouse_events.cpp
  mouse_events.hpp
  renderer_window.cpp
  renderer_window.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
add_library(om::qt::common::renderer::base ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Qt6::Gui
  drape_frontend
  map
)
