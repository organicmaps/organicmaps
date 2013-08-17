# Geomery library.

TARGET = geometry
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
  distance_on_sphere.cpp \
  screenbase.cpp \
  packer.cpp \
  robust_orientation.cpp \
  region2d/binary_operators.cpp \
  angles.cpp \

HEADERS += \
  rect2d.hpp \
  point2d.hpp \
  distance.hpp \
  distance_on_sphere.hpp \
  angles.hpp \
  screenbase.hpp \
  cellid.hpp \
  rect_intersect.hpp \
  covering.hpp \
  covering_utils.hpp \
  packer.hpp \
  pointu_to_uint64.hpp \
  simplification.hpp \
  transformations.hpp \
  tree4d.hpp \
  polygon.hpp \
  region2d.hpp \
  robust_orientation.hpp \
  any_rect2d.hpp \
  region2d/binary_operators.hpp \
  region2d/boost_concept.hpp \
  avg_vector.hpp \
