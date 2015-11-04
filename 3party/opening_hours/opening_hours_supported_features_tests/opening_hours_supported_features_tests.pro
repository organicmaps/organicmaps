TARGET = opening_hours_supported_features_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../../..
DEPENDENCIES += opening_hours \

include($$ROOT_DIR/common.pri)

OPENING_HOURS_INCLUDE = $$ROOT_DIR/3party/opening_hours
INCLUDEPATH += $$OPENING_HOURS_INCLUDE

HEADERS += $$OPENING_HOURS_INCLUDE/osm_time_range.hpp \
           $$OPENING_HOURS_INCLUDE/parse.hpp \
           $$OPENING_HOURS_INCLUDE/rules_evaluation.hpp \
           $$OPENING_HOURS_INCLUDE/rules_evaluation_private.hpp \

SOURCES += opening_hours_supported_features_tests.cpp
