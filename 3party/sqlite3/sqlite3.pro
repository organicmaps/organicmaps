TARGET = sqlite3
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

TEMPLATE = lib
CONFIG += staticlib

DEFINES += SQLITE_OMIT_LOAD_EXTENSION SQLITE_THREADSAFE=1 SQLITE_HAVE_ISNAN

SOURCES += \
    sqlite3.c \

HEADERS += \
    sqlite3ext.h \
    sqlite3.h \
