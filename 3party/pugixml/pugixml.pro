TARGET = pugixml
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

INCLUDEPATH += src

TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    src/pugixml.cpp \

HEADERS += \
    src/pugiconfig.hpp \
    src/pugixml.hpp \
