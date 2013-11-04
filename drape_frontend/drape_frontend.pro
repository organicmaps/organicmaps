TARGET = drape_frontend
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    backend_renderer.cpp \
    engine_context.cpp

HEADERS += \
    backend_renderer.hpp \
    render_thread.hpp \
    engine_context.hpp
