# Expat xml parser.

TARGET = expat
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

DEFINES += HAVE_MEMMOVE

CONFIG -= warn_on
CONFIG *= warn_off

SOURCES += \
    lib/xmlparse.c \
    lib/xmlrole.c \
    lib/xmltok.c \
    lib/xmltok_impl.c \
    lib/xmltok_ns.c \

HEADERS += \
    lib/ascii.h \
    lib/expat.h \
    lib/expat_external.h \
    lib/internal.h \
    lib/nametab.h \
    lib/xmlrole.h \
    lib/xmltok.h \
