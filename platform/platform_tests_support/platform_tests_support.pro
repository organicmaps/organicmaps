TARGET = platform_tests_support
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

SOURCES += \
    scoped_dir.cpp \
    scoped_file.cpp \

HEADERS += \
    scoped_dir.hpp \
    scoped_file.hpp \
