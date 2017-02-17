TARGET = stb_image
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    stb_image.cpp \

HEADERS += \
    stb_image.h \
