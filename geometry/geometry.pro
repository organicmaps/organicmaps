# Geomery library.

TARGET = geometry
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = base

include($$ROOT_DIR/common.pri)

SOURCES += \
  screenbase.cpp \
  packer.cpp \

HEADERS += \
  rect2d.hpp \
  point2d.hpp \
  distance.hpp \
  angles.hpp \
  screenbase.hpp \
  cellid.hpp \
  rect_intersect.hpp \
  covering.hpp \
  covering_stream_optimizer.hpp \
  packer.hpp \
  pointu_to_uint64.hpp \
  simplification.hpp \
  transformations.hpp \

