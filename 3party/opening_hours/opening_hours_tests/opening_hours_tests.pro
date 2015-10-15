
TARGET = opening_hours_tests
CONFIG += console worn_off
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../../..
DEPENDENCIES = opening_hours

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/opening_hours

SOURCES += osm_time_range_tests.cpp
SOURCES += ../osm_time_range.hpp
