TARGET = coding_sloynik_tests
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

ROOT_DIR = ../..
DEPENDENCIES = bzip2 zlib base coding coding_sloynik

include($$ROOT_DIR/common.pri)

SOURCES += $$ROOT_DIR/testing/testingmain.cpp \
    bzip2_test.cpp \
    coder_util_test.cpp \
    gzip_test.cpp \

HEADERS += \
    coder_test.hpp \

