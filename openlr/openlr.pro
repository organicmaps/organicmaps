# Match OpenLR data to MWMs.
TARGET = openlr
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
  openlr_model.cpp \
  openlr_sample.cpp \
  openlr_simple_decoder.cpp \
  openlr_simple_parser.cpp \
  road_info_getter.cpp \
  road_type_checkers.cpp \
  router.cpp \

HEADERS += \
  openlr_model.hpp \
  openlr_sample.hpp \
  openlr_simple_decoder.hpp \
  openlr_simple_parser.hpp \
  road_info_getter.hpp \
  road_type_checkers.hpp \
  router.hpp \
  way_point.hpp \
