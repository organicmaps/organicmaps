# Expat xml parser.

TARGET = expat
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..
DEPENDENCIES =

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

