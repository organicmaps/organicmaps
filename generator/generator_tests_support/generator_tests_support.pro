TARGET = generator_tests_support
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

SOURCES += \
    restriction_helpers.cpp \
    test_feature.cpp \
    test_mwm_builder.cpp \

HEADERS += \
    restriction_helpers.hpp \
    test_feature.hpp \
    test_mwm_builder.hpp \
