TARGET = zlib
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

TEMPLATE = lib
CONFIG += staticlib

DEFINES += USE_FILE32API

SOURCES += \
    adler32.c \
    compress.c \
    crc32.c \
    deflate.c \
    example.c \
    gzclose.c \
    gzlib.c \
    gzread.c \
    gzwrite.c \
    infback.c \
    inffast.c \
    inflate.c \
    inftrees.c \
    trees.c \
    uncompr.c \
    zutil.c \
    contrib/minizip/unzip.c \
    contrib/minizip/ioapi.c \

HEADERS += \
    crc32.h \
    deflate.h \
    gzguts.h \
    inffast.h \
    inffixed.h \
    inflate.h \
    inftrees.h \
    trees.h \
    zconf.h \
    zlib.h \
    zutil.h \
    contrib/minizip/unzip.h \
    contrib/minizip/ioapi.h \
