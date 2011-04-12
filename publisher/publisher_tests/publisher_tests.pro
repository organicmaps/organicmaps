TARGET = publisher_tests
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

ROOT_DIR = ../..
DEPENDENCIES = words coding base bzip2 zlib

include($$ROOT_DIR/common.pri)

SOURCES += $$ROOT_DIR/testing/testingmain.cpp \
  slof_indexer_test.cpp ../slof_indexer.cpp

HEADERS +=
