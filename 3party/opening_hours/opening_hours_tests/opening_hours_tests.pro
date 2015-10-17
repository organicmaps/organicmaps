TARGET = opening_hours_tests
CONFIG += console worn_off
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../../..
DEPENDENCIES += opening_hours \

include($$ROOT_DIR/common.pri)

OPENING_HOURS_INCLUDE = $$ROOT_DIR/3party/opening_hours
INCLUDEPATH += $$OPENING_HOURS_INCLUDE

SOURCES += osm_time_range_tests.cpp
HEADERS += $$OPENING_HOURS_INCLUDE/osm_time_range.hpp \
           $$OPENING_HOURS_INCLUDE/osm_parsers.hpp
           $$OPENING_HOURS_INCLUDE/adopted_structs.hpp
