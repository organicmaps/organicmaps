TARGET = zlib
SLOYNIK_DIR = ../..
include($$SLOYNIK_DIR/sloynik_common.pri)

TEMPLATE = lib
CONFIG += staticlib

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

