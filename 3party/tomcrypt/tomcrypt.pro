TARGET = tomcrypt
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

INCLUDEPATH += src/headers

TEMPLATE = lib
CONFIG += staticlib

DEFINES += LTC_NO_ROLC

SOURCES += \
    src/hashes/sha2/sha256.c \
    src/misc/base64/base64_decode.c \
    src/misc/base64/base64_encode.c \
    src/misc/crypt/crypt_argchk.c \

HEADERS += \
    src/headers/tomcrypt.h \
    src/headers/tomcrypt_misc.h \
