# Storage library tests.

TARGET = storage_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = storage indexer platform coding base jansson tomcrypt

include($$ROOT_DIR/common.pri)

win32*: LIBS *= -lshell32
win32-g++: LIBS *= -lpthread
macx-*: LIBS *= "-framework Foundation" "-framework IOKit"

QT *= core

HEADERS +=

SOURCES += \
  ../../testing/testingmain.cpp \
  simple_tree_test.cpp \
  country_info_test.cpp \
  storage_tests.cpp \
  guides_tests.cpp \
