# Qt common library.

TARGET = qt_common
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

QT *= core gui widgets

SOURCES += \
    helpers.cpp \
    map_widget.cpp \
    proxy_style.cpp \
    qtoglcontext.cpp \
    qtoglcontextfactory.cpp \
    scale_slider.cpp \

HEADERS += \
    helpers.hpp \
    map_widget.hpp \
    proxy_style.hpp \
    qtoglcontext.hpp \
    qtoglcontextfactory.hpp \
    scale_slider.hpp \

RESOURCES += res/resources_common.qrc
