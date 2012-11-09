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
    internal/opengl.cpp \
    vertex.cpp \
    resource_manager.cpp \
    skin.cpp \
    pen_info.cpp \
    resource_style.cpp \
    color.cpp \
    skin_loader.cpp \
    framebuffer.cpp \
    buffer_object.cpp \
    utils.cpp \
    renderbuffer.cpp \
    base_texture.cpp \
    managed_texture.cpp \
    blitter.cpp \
    clipper.cpp \
    renderer.cpp \
    geometry_renderer.cpp \
    skin_page.cpp \
    storage.cpp \
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
    rendercontext.cpp \
    line_style.cpp \
    circle_style.cpp \
    glyph_style.cpp \
    circle_element.cpp \
    packets_queue.cpp \
    display_list.cpp \
    data_formats.cpp

HEADERS += \
    internal/opengl.hpp \
    internal/gl_procedures.inl \
    vertex.hpp \
    resource_manager.hpp \
    texture.hpp \
    skin.hpp \
    skin_loader.hpp \
    pen_info.hpp \
    resource_style.hpp \
    color.hpp \
    framebuffer.hpp \
    buffer_object.hpp \
    utils.hpp \
    renderbuffer.hpp \
    base_texture.hpp \
    managed_texture.hpp \
    rendercontext.hpp \
    blitter.hpp \
    clipper.hpp \
    renderer.hpp \
    geometry_renderer.hpp \
    skin_page.hpp \
    storage.hpp \
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
    data_traits.hpp

win32* {
  SOURCES += internal/opengl_win32.cpp
} else: android*|iphone* {
  HEADERS += internal/opengl_glsl_impl.hpp
  SOURCES += internal/opengl_glsl_es2.cpp \
             internal/opengl_glsl_impl.cpp
} else {
  HEADERS += internal/opengl_glsl_impl.hpp
  SOURCES += internal/opengl_glsl_ext.cpp \
             internal/opengl_glsl_impl.cpp
}
