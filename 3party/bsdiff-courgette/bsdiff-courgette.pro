TARGET = bsdiff
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= \
  bsdiff \
  divsufsort \

HEADERS += \
  bsdiff/bsdiff.h \
  bsdiff/bsdiff_common.h \
  bsdiff/bsdiff_search.h \
  bsdiff/paged_array.h \
  divsufsort/divsufsort.h \
  divsufsort/divsufsort_private.h \


SOURCES += \
  divsufsort/divsufsort.cc \
  divsufsort/sssort.cc \
  divsufsort/trsort.cc \

