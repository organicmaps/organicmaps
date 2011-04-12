TARGET = console_sloynik
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

ROOT_DIR = ..
DEPENDENCIES = words coding base gflags bzip2 zlib

include($$ROOT_DIR/common.pri)

SOURCES += main.cpp

HEADERS +=
