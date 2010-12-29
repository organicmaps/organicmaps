# Storage library.

TARGET = storage
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = geometry coding base

include($$ROOT_DIR/common.pri)

SOURCES += \
  country.cpp \
  storage.cpp \

HEADERS += \
  defines.hpp \
  country.hpp \
  simple_tree.hpp \
  storage.hpp \
