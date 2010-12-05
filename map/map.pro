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
    storage.hpp \
    render_queue.hpp \
    render_queue_routine.hpp \

SOURCES += \
    feature_vec_model.cpp \
    framework.cpp \
    navigator.cpp \
    drawer_yg.cpp \
    draw_processor.cpp \
    storage.cpp \
    render_queue.cpp \
    render_queue_routine.cpp


!iphonesimulator-g++42 {
  !iphonedevice-g++42 {
    !bada-simulator {
      !bada-device {
        HEADERS += qgl_render_context.hpp
        SOURCES += qgl_render_context.cpp
      }
    }
  }
}
