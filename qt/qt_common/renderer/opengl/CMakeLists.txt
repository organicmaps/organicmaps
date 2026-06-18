project(qt_common_renderer_opengl)

set(SRC
  opengl_context.cpp
  opengl_context.hpp
  opengl_context_factory.cpp
  opengl_context_factory.hpp
  opengl_window.cpp
  opengl_window.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
add_library(om::qt::common::renderer::opengl ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Qt6::OpenGL
  Qt6::Gui
  drape
  om::qt::common::renderer::base
)

qt_add_resources(${PROJECT_NAME} "shaders"
  PREFIX
    "/common/renderer/opengl"
  FILES
    shaders/gl_150.fsh.glsl
    shaders/gl_150.vsh.glsl
    shaders/gles_300.fsh.glsl
    shaders/gles_300.vsh.glsl
)
