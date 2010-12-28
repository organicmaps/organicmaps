# Storage library.

TARGET = storage
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = indexer platform geometry coding base

include($$ROOT_DIR/common.pri)

HEADERS += \
  country.hpp \
  defines.hpp \
  simple_tree.hpp \
  storage.hpp \

SOURCES += \
  country.cpp \
  storage.cpp \
