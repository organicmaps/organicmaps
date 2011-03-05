TARGET = publisher_tests
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SLOYNIK_DIR = ../..
DEPENDENCIES = gflags bzip2 zlib base coding coding_sloynik words

include($$SLOYNIK_DIR/sloynik_common.pri)

SOURCES += $$SLOYNIK_DIR/testing/testingmain.cpp \
  slof_indexer_test.cpp ../slof_indexer.cpp

HEADERS +=
