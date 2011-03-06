TARGET = console_sloynik
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

ROOT_DIR = ..
DEPENDENCIES = gflags bzip2 zlib base coding words

include($$ROOT_DIR/common.pri)

SOURCES += main.cpp

HEADERS +=
