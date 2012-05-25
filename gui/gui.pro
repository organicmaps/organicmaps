# GUI library on top of YG

TARGET = gui
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = yg geometry coding base expat

include($$ROOT_DIR/common.pri)

HEADERS += \
    controller.hpp\
    element.hpp \
    button.hpp \

SOURCES += \
    controller.cpp \
    element.cpp \
    button.cpp \






