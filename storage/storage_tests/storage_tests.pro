# Storage library tests.

TARGET = storage_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = storage indexer platform coding base jansson tomcrypt expat

include($$ROOT_DIR/common.pri)

win32*: LIBS *= -lshell32
macx*: LIBS *= "-framework Foundation" "-framework IOKit"

QT *= core

HEADERS +=

SOURCES += \
  ../../testing/testingmain.cpp \
  country_test.cpp \
  simple_tree_test.cpp \
  country_info_test.cpp \
  generate_langs.cpp \
