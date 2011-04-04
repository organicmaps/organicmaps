TARGET = tomcrypt
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

INCLUDEPATH += src/headers

TEMPLATE = lib
CONFIG += staticlib

DEFINES += LTC_NO_ROLC

SOURCES += \
    src/hashes/sha2/sha224.c \
    src/hashes/sha2/sha256.c \
    src/hashes/sha2/sha384.c \
    src/hashes/sha2/sha512.c \

HEADERS += \
    src/headers/tomcrypt.h \
