TARGET = utils_tests
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

ROOT_DIR = ../..
DEPENDENCIES = protobuf base coding utils

include($$ROOT_DIR/common.pri)

SOURCES += $$ROOT_DIR/testing/testingmain.cpp \
  dummy_utils_test.cpp

HEADERS += \

