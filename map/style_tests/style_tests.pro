TARGET = style_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += ../../3party/protobuf/src

ROOT_DIR = ../..
DEPENDENCIES = map indexer platform geometry coding base expat protobuf

macx-*: LIBS *= "-framework IOKit"

include($$ROOT_DIR/common.pri)

QT *= core

SOURCES += \
  ../../testing/testingmain.cpp \
  classificator_tests.cpp \
  dashes_test.cpp \
  style_symbols_consistency_test.cpp \
