TARGET = coding_sloynik_tests
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SLOYNIK_DIR = ../..
DEPENDENCIES = bzip2 zlib base coding coding_sloynik

include($$SLOYNIK_DIR/sloynik_common.pri)

SOURCES += $$SLOYNIK_DIR/testing/testingmain.cpp \
    bzip2_test.cpp \
    coder_util_test.cpp \
    gzip_test.cpp \

HEADERS += \
    coder_test.hpp \

