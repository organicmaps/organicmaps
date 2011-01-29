# Geometry library tests.

TARGET = geometry_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = indexer geometry base

include($$ROOT_DIR/common.pri)

HEADERS += \
  equality.hpp \
  large_polygon.hpp \

SOURCES += \
  ../../testing/testingmain.cpp \
  distance_test.cpp \
  angle_test.cpp \
  common_test.cpp \
  screen_test.cpp \
  cellid_test.cpp \
  intersect_test.cpp \
  point_test.cpp \
  packer_test.cpp \
  segments_intersect_test.cpp \
  covering_test.cpp \
  covering_stream_optimizer_test.cpp \
  pointu_to_uint64_test.cpp \
  simplification_test.cpp \
  transformations_test.cpp \
  tree_test.cpp \
  polygon_test.cpp \
  region_test.cpp \
