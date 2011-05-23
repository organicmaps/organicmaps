# Search library.

TARGET = search
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = indexer geometry coding base

include($$ROOT_DIR/common.pri)

HEADERS += \
    query.hpp \
    search_processor.hpp \
    string_match.hpp \
    delimiters.hpp \

SOURCES += \
    query.cpp \
    search_processor.cpp \
    string_match.cpp \
    delimiters.cpp \
