TARGET = sdf_image
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    sdf_image.cpp \

HEADERS += \
    sdf_image.h \
