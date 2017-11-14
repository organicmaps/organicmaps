TARGET = indexer_tests_support
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

SOURCES += \
    helpers.cpp \
    test_with_classificator.cpp \
    test_with_custom_mwms.cpp \

HEADERS += \
    helpers.hpp \
    test_with_classificator.hpp \
    test_with_custom_mwms.hpp \
