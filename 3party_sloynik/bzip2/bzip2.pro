TARGET = bzip2
SLOYNIK_DIR = ../..
include($$SLOYNIK_DIR/sloynik_common.pri)

TEMPLATE = lib
CONFIG += staticlib
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
