TARGET = generator_tests_support
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

SOURCES += \
    routing_helpers.cpp \
    test_feature.cpp \
    test_mwm_builder.cpp \

HEADERS += \
    routing_helpers.hpp \
    test_feature.hpp \
    test_mwm_builder.hpp \
