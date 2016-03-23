TARGET = oauthcpp
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

INCLUDEPATH += src include

TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    src/base64.cpp \
    src/HMAC_SHA1.cpp \
    src/SHA1.cpp \
    src/urlencode.cpp \
    src/liboauthcpp.cpp \

HEADERS += \
    include/liboauthcpp/liboauthcpp.h \
