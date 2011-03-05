TARGET = words_tests
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SLOYNIK_DIR = ../..
DEPENDENCIES = bzip2 zlib base coding coding_sloynik words

include($$SLOYNIK_DIR/sloynik_common.pri)

SOURCES += $$SLOYNIK_DIR/testing/testingmain.cpp \
  sorted_index_test.cpp \

HEADERS += \
  dictionary_mock.hpp \
