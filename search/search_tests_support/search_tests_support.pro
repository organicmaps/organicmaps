TARGET = search_tests_support
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

SOURCES += \
    test_search_engine.cpp \
    test_search_request.cpp \

HEADERS += \
    test_search_engine.hpp \
    test_search_request.hpp \
