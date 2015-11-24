TARGET = generator_tests_support
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/expat/lib

HEADERS += \
    test_mwm_builder.hpp \

SOURCES += \
    test_mwm_builder.cpp \
