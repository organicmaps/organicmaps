TARGET = utils_tests
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SLOYNIK_DIR = ../..
DEPENDENCIES = protobuf base coding coding_sloynik utils

include($$SLOYNIK_DIR/sloynik_common.pri)

SOURCES += $$SLOYNIK_DIR/testing/testingmain.cpp \
  dummy_utils_test.cpp

HEADERS += \

