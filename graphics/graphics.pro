# Graphics library.

TARGET = graphics
TEMPLATE = lib
CONFIG += staticlib
DEFINES += GRAPHICS_LIBRARY

ROOT_DIR = ..
DEPENDENCIES = indexer geometry platform coding base freetype fribidi expat

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
    skin.cpp \
    pen_info.cpp \
    resource_style.cpp \
    color.cpp \
    skin_loader.cpp \
    skin_page.cpp \
    glyph_cache.cpp \
    glyph_cache_impl.cpp \
    ft2_debug.cpp \
    geometry_batcher.cpp \
    text_renderer.cpp \
    path_renderer.cpp \
    shape_renderer.cpp \
    circle_info.cpp \
    area_renderer.cpp \
    font_desc.cpp \
    glyph_layout.cpp \
    text_element.cpp \
    text_path.cpp \
    overlay.cpp \
    overlay_element.cpp \
    symbol_element.cpp \
    overlay_renderer.cpp \
    composite_overlay_element.cpp \
    path_text_element.cpp \
    straight_text_element.cpp \
    line_style.cpp \
    circle_style.cpp \
    glyph_style.cpp \
    circle_element.cpp \
    packets_queue.cpp \
    display_list.cpp \
    data_formats.cpp \
    image_info.cpp \
    image_renderer.cpp \
    display_list_renderer.cpp

HEADERS += \
    opengl/opengl.hpp \
    opengl/gl_procedures.inl \
    opengl/vertex.hpp \
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
    skin.hpp \
    skin_loader.hpp \
    pen_info.hpp \
    resource_style.hpp \
    color.hpp \
    skin_page.hpp \
    render_target.hpp \
    glyph_cache.hpp \
    data_formats.hpp \
    glyph_cache_impl.hpp \
    ft2_debug.hpp \
    text_renderer.hpp \
    geometry_batcher.hpp \
    screen.hpp \
    defines.hpp \
    path_renderer.hpp \
    shape_renderer.hpp \
    circle_info.hpp \
    area_renderer.hpp \
    font_desc.hpp \
    glyph_layout.hpp \
    text_element.hpp \
    text_path.hpp \
    overlay.hpp \
    overlay_element.hpp \
    symbol_element.hpp \
    overlay_renderer.hpp \
    composite_overlay_element.hpp \
    path_text_element.hpp \
    straight_text_element.hpp \
    agg_traits.hpp \
    circle_element.hpp \
    packets_queue.hpp \
    display_list.hpp \
    image_renderer.hpp \
    image_info.hpp \
    display_list_renderer.hpp
    render_context.hpp \

win32* {
  SOURCES += opengl/opengl_win32.cpp
} else: android*|iphone* {
  HEADERS +=
  SOURCES += opengl/opengl_es2.cpp
} else {
  HEADERS +=
  SOURCES += opengl/opengl_ext.cpp
}
