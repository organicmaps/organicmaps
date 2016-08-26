TARGET = indexer_tests_support
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

SOURCES += \
    helpers.cpp \

HEADERS += \
    helpers.hpp \
