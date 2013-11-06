TARGET = drape_frontend
TEMPLATE = lib
CONFIG += staticlib

DEPENDENCIES = base
ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

SOURCES += \
    backend_renderer.cpp \
    engine_context.cpp \
    memory_feature_index.cpp

HEADERS += \
    backend_renderer.hpp \
    render_thread.hpp \
    engine_context.hpp \
    memory_feature_index.hpp \
    tile_info.hpp
