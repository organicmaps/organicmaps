TARGET = jansson
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

INCLUDEPATH += src

TEMPLATE = lib
CONFIG += staticlib
SOURCES += \
    src/dump.c \
    src/hashtable.c \
    src/load.c \
    src/strbuffer.c \
    src/utf.c \
    src/value.c \
    src/memory.c \
    src/error.c \

HEADERS += \
    myjansson.hpp \
    src/jansson.h \
    src/jansson_private.h \
    src/hashtable.h \
    src/strbuffer.h \
    src/utf.h \
    src/util.h \
    src/jansson_config.h \
