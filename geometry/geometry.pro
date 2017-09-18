# Geomery library.

TARGET = geometry
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
  algorithm.cpp \
  angles.cpp \
  bbox.cpp \
  cbox.cpp \
  clipping.cpp \
  convex_hull.cpp \
  distance_on_sphere.cpp \
  latlon.cpp \
  line2d.cpp \
  mercator.cpp \
  nearby_points_sweeper.cpp \
  packer.cpp \
  region2d/binary_operators.cpp \
  robust_orientation.cpp \
  screenbase.cpp \
  segment2d.cpp \
  spline.cpp \
  triangle2d.cpp \

HEADERS += \
  algorithm.hpp \
  angles.hpp \
  any_rect2d.hpp \
  avg_vector.hpp \
  bbox.hpp \
  cbox.hpp \
  cellid.hpp \
  clipping.hpp \
  convex_hull.hpp \
  covering.hpp \
  covering_utils.hpp \
  dbox.hpp \
  distance.hpp \
  distance_on_sphere.hpp \
  latlon.hpp \
  line2d.hpp \
  mercator.hpp \
  nearby_points_sweeper.hpp \
  packer.hpp \
  point2d.hpp \
  pointu_to_uint64.hpp \
  polygon.hpp \
  polyline2d.hpp \
  rect2d.hpp \
  rect_intersect.hpp \
  region2d.hpp \
  region2d/binary_operators.hpp \
  region2d/boost_concept.hpp \
  robust_orientation.hpp \
  screenbase.hpp \
  segment2d.hpp \
  simplification.hpp \
  spline.hpp \
  transformations.hpp \
  tree4d.hpp \
  triangle2d.hpp \
