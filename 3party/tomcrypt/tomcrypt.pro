TARGET = tomcrypt
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

INCLUDEPATH += src/headers

TEMPLATE = lib
CONFIG += staticlib

DEFINES += LTC_NO_ROLC

SOURCES += \
    src/hashes/sha2/sha256.c \

HEADERS += \
    src/headers/tomcrypt.h \
