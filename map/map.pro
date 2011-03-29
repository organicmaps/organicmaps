# Map library.

TARGET = map
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = yg indexer geometry coding base expat

include($$ROOT_DIR/common.pri)

HEADERS += \
    framework.hpp \
    feature_vec_model.hpp \
    events.hpp \
    navigator.hpp \
    drawer_yg.hpp \
    draw_processor.hpp \
    draw_info.hpp \
    window_handle.hpp \
    render_queue.hpp \
    render_queue_routine.hpp \
    information_display.hpp \
    settings.hpp \
    locator.hpp

SOURCES += \
    feature_vec_model.cpp \
    framework.cpp \
    navigator.cpp \
    drawer_yg.cpp \
    draw_processor.cpp \
    render_queue.cpp \
    render_queue_routine.cpp \
    information_display.cpp \
    settings.cpp \
    locator.cpp

!iphone*:!bada* {
  HEADERS += qgl_render_context.hpp
  SOURCES += qgl_render_context.cpp
  QT += opengl
}
