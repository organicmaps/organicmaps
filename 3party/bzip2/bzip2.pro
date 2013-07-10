TARGET = bzip2
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

TEMPLATE = lib
CONFIG += staticlib warn_off
SOURCES += \
    blocksort.c \
    bzlib.c \
    compress.c \
    crctable.c \
    decompress.c \
    huffman.c \
    randtable.c \

HEADERS += \
    bzlib.h \
    bzlib_private.h \
