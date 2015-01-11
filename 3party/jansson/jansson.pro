TARGET = jansson
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

INCLUDEPATH += src

TEMPLATE = lib
CONFIG += staticlib
QMAKE_CFLAGS_WARN_ON += -Wno-unused-function

SOURCES += \
    src/dump.c \
    src/hashtable.c \
    src/load.c \
    src/strbuffer.c \
    src/utf.c \
    src/value.c \
    src/memory.c \
    src/error.c \
    src/strconv.c \
    jansson_handle.cpp \

HEADERS += \
    myjansson.hpp \
    src/jansson.h \
    src/jansson_private.h \
    src/hashtable.h \
    src/strbuffer.h \
    src/utf.h \
    src/jansson_config.h \
    jansson_handle.hpp \
