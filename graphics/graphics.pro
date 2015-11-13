# Graphics library.

TARGET = graphics
TEMPLATE = lib
CONFIG += staticlib warn_on
DEFINES += GRAPHICS_LIBRARY

ROOT_DIR = ..

INCLUDEPATH += $$ROOT_DIR/3party/freetype/include $$ROOT_DIR/3party/agg

include($$ROOT_DIR/common.pri)

SOURCES += \
    opengl/framebuffer.cpp \
    opengl/opengl.cpp \
    opengl/buffer_object.cpp \
    opengl/renderbuffer.cpp \
    opengl/base_texture.cpp \
    opengl/managed_texture.cpp \
    opengl/renderer.cpp \
    opengl/vertex.cpp \
    opengl/route_vertex.cpp \
    opengl/clipper.cpp \
    opengl/geometry_renderer.cpp \
    opengl/shader.cpp \
    opengl/program.cpp \
    opengl/defines_conv.cpp \
    opengl/program_manager.cpp \
    opengl/gl_render_context.cpp \
    opengl/storage.cpp \
    blitter.cpp \
    resource_manager.cpp \
    pen.cpp \
    resource.cpp \
    color.cpp \
    skin_loader.cpp \
    resource_cache.cpp \
    glyph_cache.cpp \
    freetype.cpp \
    glyph_cache_impl.cpp \
    geometry_batcher.cpp \
    text_renderer.cpp \
    path_renderer.cpp \
    shape_renderer.cpp \
    circle.cpp \
    area_renderer.cpp \
    font_desc.cpp \
    glyph_layout.cpp \
    text_element.cpp \
    text_path.cpp \
    overlay.cpp \
    overlay_element.cpp \
    symbol_element.cpp \
    overlay_renderer.cpp \
    path_text_element.cpp \
    straight_text_element.cpp \
    glyph.cpp \
    circle_element.cpp \
    packets_queue.cpp \
    display_list.cpp \
    data_formats.cpp \
    image.cpp \
    image_renderer.cpp \
    display_list_renderer.cpp \
    vertex_decl.cpp \
    render_context.cpp \
    coordinates.cpp \
    render_target.cpp \
    defines.cpp \
    icon.cpp \
    brush.cpp \
    pipeline_manager.cpp \
    geometry_pipeline.cpp \
    path_view.cpp \
    circled_symbol.cpp \
    uniforms_holder.cpp

HEADERS += \
    opengl/opengl.hpp \
    opengl/gl_procedures.inl \
    opengl/vertex.hpp \
    opengl/route_vertex.hpp \
    opengl/texture.hpp \
    opengl/framebuffer.hpp \
    opengl/buffer_object.hpp \
    opengl/renderbuffer.hpp \
    opengl/base_texture.hpp \
    opengl/managed_texture.hpp \
    opengl/gl_render_context.hpp \
    opengl/clipper.hpp \
    opengl/renderer.hpp \
    opengl/geometry_renderer.hpp \
    opengl/data_traits.hpp \
    opengl/storage.hpp \
    opengl/shader.hpp \
    opengl/program.hpp \
    opengl/defines_conv.hpp \
    opengl/program_manager.hpp \
    blitter.hpp \
    resource_manager.hpp \
    skin_loader.hpp \
    pen.hpp \
    resource.hpp \
    color.hpp \
    resource_cache.hpp \
    render_target.hpp \
    glyph_cache.hpp \
    data_formats.hpp \
    glyph_cache_impl.hpp \
    freetype.hpp \
    text_renderer.hpp \
    geometry_batcher.hpp \
    screen.hpp \
    defines.hpp \
    path_renderer.hpp \
    shape_renderer.hpp \
    circle.hpp \
    area_renderer.hpp \
    font_desc.hpp \
    glyph_layout.hpp \
    text_element.hpp \
    text_path.hpp \
    overlay.hpp \
    overlay_element.hpp \
    symbol_element.hpp \
    overlay_renderer.hpp \
    path_text_element.hpp \
    straight_text_element.hpp \
    agg_traits.hpp \
    circle_element.hpp \
    packets_queue.hpp \
    display_list.hpp \
    image_renderer.hpp \
    image.hpp \
    display_list_renderer.hpp \
    vertex_decl.hpp \
    render_context.hpp \
    coordinates.hpp \
    icon.hpp \
    glyph.hpp \
    brush.hpp \
    geometry_pipeline.hpp \
    pipeline_manager.hpp \
    vertex_stream.hpp \
    path_view.hpp \
    path.hpp \
    depth_constants.hpp \
    circled_symbol.hpp \
    uniforms_holder.hpp

win32* {
  SOURCES += opengl/opengl_win32.cpp
} else: android*|iphone*|tizen* {
  HEADERS +=
  SOURCES += opengl/opengl_es2.cpp
} else {
  HEADERS +=
  CONFIG(OMIM_OS_MAEMO) {
    SOURCES += opengl/opengl_es2.cpp
  } else {
    SOURCES += opengl/opengl_ext.cpp
  }
}
