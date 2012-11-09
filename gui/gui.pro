# GUI library on top of Graphics

TARGET = gui
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = graphics geometry coding base expat

include($$ROOT_DIR/common.pri)

HEADERS += \
    controller.hpp\
    element.hpp \
    button.hpp \
    text_view.hpp \

SOURCES += \
    controller.cpp \
    element.cpp \
    button.cpp \
    text_view.cpp \
