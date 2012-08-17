# Animation library

TARGET = anim
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = geometry coding base expat

include($$ROOT_DIR/common.pri)

HEADERS += \
    controller.hpp \
    task.hpp

SOURCES += \
    controller.cpp \
    task.cpp



