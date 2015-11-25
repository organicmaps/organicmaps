# Pugi xml parser with XML DOM & XPath support.

TARGET = pugixml
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

SOURCES += \
    src/pugixml.cpp \

HEADERS += \
    src/pugixml.hpp \
    src/pugiconfig.hpp \
