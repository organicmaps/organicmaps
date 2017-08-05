TARGET = software_renderer
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/protobuf/protobuf/src
INCLUDEPATH *= $$ROOT_DIR/3party/expat/lib
INCLUDEPATH *= $$ROOT_DIR/3party/freetype/include

SOURCES += \
    cpu_drawer.cpp \
    software_renderer.cpp \
    proto_to_styles.cpp \
    text_engine.cpp \
    feature_styler.cpp \
    glyph_cache.cpp \
    glyph_cache_impl.cpp \
    geometry_processors.cpp \
    feature_processor.cpp \
    default_font.cpp \

HEADERS += \
    cpu_drawer.hpp \
    software_renderer.hpp \
    proto_to_styles.hpp \
    text_engine.h \
    point.h \
    rect.h \
    path_info.hpp \
    area_info.hpp \
    frame_image.hpp \
    feature_styler.hpp \
    glyph_cache.hpp \
    glyph_cache_impl.hpp \
    freetype.hpp \
    circle_info.hpp \
    pen_info.hpp \
    icon_info.hpp \
    brush_info.hpp \
    geometry_processors.hpp \
    feature_processor.hpp \
