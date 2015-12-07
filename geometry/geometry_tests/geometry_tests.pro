# Geometry library tests.

TARGET = geometry_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = geometry base

include($$ROOT_DIR/common.pri)

HEADERS += \
  equality.hpp \
  large_polygon.hpp \
  test_regions.hpp \

SOURCES += \
  ../../testing/testingmain.cpp \
  angle_test.cpp \
  anyrect_test.cpp \
  cellid_test.cpp \
  common_test.cpp \
  covering_test.cpp \
  distance_on_sphere_test.cpp \
  distance_test.cpp \
  intersect_test.cpp \
  latlon_test.cpp \
  mercator_test.cpp \
  packer_test.cpp \
  point_test.cpp \
  pointu_to_uint64_test.cpp \
  polygon_test.cpp \
  rect_test.cpp \
  region2d_binary_op_test.cpp \
  region_test.cpp \
  robust_test.cpp \
  screen_test.cpp \
  segments_intersect_test.cpp \
  simplification_test.cpp \
  spline_test.cpp \
  transformations_test.cpp \
  tree_test.cpp \
  vector_test.cpp \
