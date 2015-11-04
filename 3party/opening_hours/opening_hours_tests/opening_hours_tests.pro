TARGET = opening_hours_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../../..
DEPENDENCIES += opening_hours \

include($$ROOT_DIR/common.pri)

OPENING_HOURS_INCLUDE = $$ROOT_DIR/3party/opening_hours
INCLUDEPATH += $$OPENING_HOURS_INCLUDE

HEADERS += $$OPENING_HOURS_INCLUDE/opening_hours.hpp \
           $$OPENING_HOURS_INCLUDE/parse_opening_hours.hpp \
           $$OPENING_HOURS_INCLUDE/rules_evaluation.hpp \
           $$OPENING_HOURS_INCLUDE/rules_evaluation_private.hpp

SOURCES += opening_hours_tests.cpp
