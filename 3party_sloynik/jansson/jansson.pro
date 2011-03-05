TARGET = jansson
SLOYNIK_DIR = ../..
include($$SLOYNIK_DIR/sloynik_common.pri)

TEMPLATE = lib
CONFIG += staticlib
SOURCES += \
  src/dump.c \
  src/hashtable.c \
  src/load.c \
  src/strbuffer.c \
  src/utf.c \
  src/value.c \

HEADERS += \
  myjansson.hpp \
  src/jansson.h \
  src/jansson_private.h \
  src/hashtable.h \
  src/strbuffer.h \
  src/utf.h \
  src/util.h \
