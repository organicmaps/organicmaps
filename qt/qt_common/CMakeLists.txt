project(qt_common)

# find_package(Qt6 REQUIRED COMPONENTS Quick QuickWidgets)

QT6_ADD_RESOURCES(RESOURCES res/resources_common.qrc)

set_property(SOURCE qrc_resources_common.cpp PROPERTY SKIP_AUTOGEN ON)

set(SRC
  helpers.cpp
  helpers.hpp
  map_widget.cpp
  map_widget.hpp
  proxy_style.cpp
  proxy_style.hpp
  qtoglcontext.cpp
  qtoglcontext.hpp
  qtoglcontextfactory.cpp
  qtoglcontextfactory.hpp
  scale_slider.cpp
  scale_slider.hpp
  spinner.cpp
  spinner.hpp
  text_dialog.cpp
  text_dialog.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC} ${RESOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    map
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Quick
    Qt6::QuickWidgets
)

# qt_add_qml_module(${PROJECT_NAME}
#     URI fbitem
#     QML_FILES cloud.qml
#     RESOURCE_PREFIX /qquickwidgetversuswindow_opengl
#     NO_RESOURCE_TARGET_PATH
#     DEPENDENCIES QtQuick
# )