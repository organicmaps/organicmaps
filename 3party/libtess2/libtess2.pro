TARGET = tess2
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

CONFIG -= warn_on
CONFIG *= warn_off

INCLUDEPATH *= Include

HEADERS += \
  Include/tesselator.h \
  Source/bucketalloc.h \
  Source/dict.h \
  Source/geom.h \
  Source/mesh.h \
  Source/priorityq.h \
  Source/sweep.h \
  Source/tess.h \

SOURCES += \
  Source/bucketalloc.c \
  Source/dict.c \
  Source/geom.c \
  Source/mesh.c \
  Source/priorityq.c \
  Source/sweep.c \
  Source/tess.c \
