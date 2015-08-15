TARGET = minizip
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

CONFIG -= warn_on
CONFIG *= warn_off

TEMPLATE = lib
CONFIG += staticlib

DEFINES += USE_FILE32API NOCRYPT
INCLUDEPATH += .

SOURCES += \
    unzip.c \
    ioapi.c \
    zip.c \

HEADERS += \
    unzip.h \
    ioapi.h \
    zip.h \
