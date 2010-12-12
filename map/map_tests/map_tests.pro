# Map library tests.

TARGET = map_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES =  map indexer platform geometry coding base expat

include($$ROOT_DIR/common.pri)

QT *= core network

win32 {
  LIBS += -lShell32
}

SOURCES += \
  ../../testing/testingmain.cpp \
  navigator_test.cpp \
  map_foreach_test.cpp \
  simple_tree_test.cpp \
