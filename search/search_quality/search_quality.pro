# Search quality library.

TARGET = search_quality
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

HEADERS += \
    helpers.hpp \
    sample.hpp \

SOURCES += \
    helpers.cpp \
    sample.cpp \
