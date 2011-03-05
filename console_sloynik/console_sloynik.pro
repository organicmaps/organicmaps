TARGET = console_sloynik
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SLOYNIK_DIR = ..
DEPENDENCIES = gflags bzip2 zlib base coding coding_sloynik words

include($$SLOYNIK_DIR/sloynik_common.pri)

SOURCES += main.cpp

HEADERS +=
